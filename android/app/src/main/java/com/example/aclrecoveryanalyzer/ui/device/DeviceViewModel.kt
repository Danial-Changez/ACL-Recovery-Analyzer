package com.example.aclrecoveryanalyzer.ui.device

import android.bluetooth.BluetoothDevice
import androidx.lifecycle.ViewModel
import com.example.aclrecoveryanalyzer.data.ble.BleManager
import com.example.aclrecoveryanalyzer.data.ble.ConnectionState
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.StateFlow
import javax.inject.Inject

@HiltViewModel
class DeviceViewModel @Inject constructor(
    private val bleManager: BleManager
) : ViewModel() {

    val connectionState: StateFlow<ConnectionState> = bleManager.connectionState
    val scannedDevices: StateFlow<List<BluetoothDevice>> = bleManager.scannedDevices
    val isBluetoothEnabled: Boolean get() = bleManager.isBluetoothEnabled

    fun startScan() {
        bleManager.startScan()
    }

    fun stopScan() {
        bleManager.stopScan()
    }

    fun connect(device: BluetoothDevice) {
        bleManager.connect(device)
    }

    fun disconnect() {
        bleManager.disconnect()
    }
}
