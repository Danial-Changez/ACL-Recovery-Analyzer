package com.example.aclrecoveryanalyzer.data.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.os.Build
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import com.example.aclrecoveryanalyzer.domain.model.RehabPacket
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import javax.inject.Inject
import javax.inject.Singleton

enum class ConnectionState {
    DISCONNECTED,
    SCANNING,
    CONNECTING,
    CONNECTED,
    RECONNECTING
}

/**
 * Manages BLE scanning, connection, and data streaming for the ACL-Rehab device.
 *
 * Subscribes to GATT notifications on the live data characteristic and emits
 * parsed [RehabPacket]s via [packets]. Handles automatic reconnection on
 * unexpected disconnects.
 */
@Singleton
class BleManager @Inject constructor(
    @param:ApplicationContext private val context: Context,
    private val packetParser: RehabPacketParser
) {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)

    private val bluetoothManager =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.adapter

    private var bluetoothGatt: BluetoothGatt? = null
    private var reconnectAttempts = 0
    private var shouldReconnect = false
    private var lastDevice: BluetoothDevice? = null

    private val _connectionState = MutableStateFlow(ConnectionState.DISCONNECTED)
    val connectionState: StateFlow<ConnectionState> = _connectionState.asStateFlow()

    private val _packets = MutableSharedFlow<RehabPacket>(extraBufferCapacity = 64)
    val packets: SharedFlow<RehabPacket> = _packets.asSharedFlow()

    private val _scannedDevices = MutableStateFlow<List<BluetoothDevice>>(emptyList())
    val scannedDevices: StateFlow<List<BluetoothDevice>> = _scannedDevices.asStateFlow()

    private val _error = MutableSharedFlow<String>(extraBufferCapacity = 8)
    val error: SharedFlow<String> = _error.asSharedFlow()

    val isBluetoothEnabled: Boolean
        get() = bluetoothAdapter?.isEnabled == true

    private val scanCallback = object : ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            val currentList = _scannedDevices.value
            if (currentList.none { it.address == device.address }) {
                _scannedDevices.value = currentList + device
            }
        }

        override fun onScanFailed(errorCode: Int) {
            _connectionState.value = ConnectionState.DISCONNECTED
            _error.tryEmit("BLE scan failed (error $errorCode)")
        }
    }

    /**
     * Starts a BLE scan filtered by device name. Automatically stops after
     * [BleConstants.SCAN_TIMEOUT_MS].
     */
    @SuppressLint("MissingPermission")
    fun startScan() {
        val scanner = bluetoothAdapter?.bluetoothLeScanner
        if (scanner == null) {
            _error.tryEmit("Bluetooth is not available")
            return
        }

        _scannedDevices.value = emptyList()
        _connectionState.value = ConnectionState.SCANNING

        val filter = ScanFilter.Builder()
            .setDeviceName(BleConstants.DEVICE_NAME)
            .build()

        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        scanner.startScan(listOf(filter), settings, scanCallback)

        scope.launch {
            delay(BleConstants.SCAN_TIMEOUT_MS)
            stopScan()
        }
    }

    @SuppressLint("MissingPermission")
    fun stopScan() {
        bluetoothAdapter?.bluetoothLeScanner?.stopScan(scanCallback)
        if (_connectionState.value == ConnectionState.SCANNING) {
            _connectionState.value = ConnectionState.DISCONNECTED
        }
    }

    @SuppressLint("MissingPermission")
    fun connect(device: BluetoothDevice) {
        stopScan()
        _connectionState.value = ConnectionState.CONNECTING
        shouldReconnect = true
        reconnectAttempts = 0
        lastDevice = device

        bluetoothGatt?.close()
        bluetoothGatt = device.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
    }

    @SuppressLint("MissingPermission")
    fun disconnect() {
        shouldReconnect = false
        reconnectAttempts = 0
        bluetoothGatt?.let { gatt ->
            gatt.disconnect()
            gatt.close()
        }
        bluetoothGatt = null
        lastDevice = null
        _connectionState.value = ConnectionState.DISCONNECTED
    }

    private val gattCallback = object : BluetoothGattCallback() {

        @SuppressLint("MissingPermission")
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    reconnectAttempts = 0
                    gatt.requestMtu(27)
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    gatt.close()
                    bluetoothGatt = null

                    if (shouldReconnect && reconnectAttempts < BleConstants.MAX_RECONNECT_ATTEMPTS) {
                        _connectionState.value = ConnectionState.RECONNECTING
                        attemptReconnect()
                    } else {
                        _connectionState.value = ConnectionState.DISCONNECTED
                        if (shouldReconnect) {
                            _error.tryEmit("Lost connection after ${BleConstants.MAX_RECONNECT_ATTEMPTS} reconnect attempts")
                            shouldReconnect = false
                        }
                    }
                }
            }
        }

        @SuppressLint("MissingPermission")
        override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
            gatt.discoverServices()
        }

        @SuppressLint("MissingPermission")
        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                _error.tryEmit("Service discovery failed")
                return
            }

            val service = gatt.getService(BleConstants.SERVICE_UUID)
            if (service == null) {
                _error.tryEmit("ACL-Rehab service not found on device")
                return
            }

            val liveChar = service.getCharacteristic(BleConstants.LIVE_DATA_CHAR_UUID)
            if (liveChar == null) {
                _error.tryEmit("Live data characteristic not found")
                return
            }

            gatt.setCharacteristicNotification(liveChar, true)

            val descriptor = liveChar.getDescriptor(BleConstants.CCCD_UUID)
            if (descriptor != null) {
                @Suppress("DEPRECATION")
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    gatt.writeDescriptor(descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
                } else {
                    descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                    gatt.writeDescriptor(descriptor)
                }
            } else {
                _connectionState.value = ConnectionState.CONNECTED
            }
        }

        @SuppressLint("MissingPermission")
        override fun onDescriptorWrite(
            gatt: BluetoothGatt,
            descriptor: BluetoothGattDescriptor,
            status: Int
        ) {
            if (descriptor.uuid == BleConstants.CCCD_UUID) {
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    _connectionState.value = ConnectionState.CONNECTED
                } else {
                    _error.tryEmit("Failed to enable notifications (status $status)")
                }
            }
        }

        @Deprecated("Kept for backward compatibility with minSdk 26")
        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        ) {
            if (characteristic.uuid == BleConstants.LIVE_DATA_CHAR_UUID) {
                @Suppress("DEPRECATION")
                packetParser.parse(characteristic.value)?.let { _packets.tryEmit(it) }
            }
        }

        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray
        ) {
            if (characteristic.uuid == BleConstants.LIVE_DATA_CHAR_UUID) {
                packetParser.parse(value)?.let { _packets.tryEmit(it) }
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun attemptReconnect() {
        val device = lastDevice ?: return
        reconnectAttempts++

        scope.launch {
            delay(BleConstants.RECONNECT_DELAY_MS)
            if (shouldReconnect) {
                bluetoothGatt = device.connectGatt(
                    context, false, gattCallback, BluetoothDevice.TRANSPORT_LE
                )
            }
        }
    }
}
