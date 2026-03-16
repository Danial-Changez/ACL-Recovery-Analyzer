package com.example.aclrecoveryanalyzer.ui.live

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.aclrecoveryanalyzer.data.ble.BleManager
import com.example.aclrecoveryanalyzer.data.ble.ConnectionState
import com.example.aclrecoveryanalyzer.domain.model.RehabPacket
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class LiveSessionViewModel @Inject constructor(
    private val bleManager: BleManager
) : ViewModel() {

    val connectionState: StateFlow<ConnectionState> = bleManager.connectionState

    private val _latestPacket = MutableStateFlow<RehabPacket?>(null)
    val latestPacket: StateFlow<RehabPacket?> = _latestPacket.asStateFlow()

    private val _peakKneeAngle = MutableStateFlow(0f)
    val peakKneeAngle: StateFlow<Float> = _peakKneeAngle.asStateFlow()

    /** Estimated remaining battery time in minutes, null until sufficient data is collected. */
    private val _batteryEstimateMin = MutableStateFlow<Float?>(null)
    val batteryEstimateMin: StateFlow<Float?> = _batteryEstimateMin.asStateFlow()

    /** Tracks drain rate by recording the first and most recent battery readings. */
    private var firstBatteryPercent: Int? = null
    private var firstBatteryTimeMs: Long = 0L
    private var lastBatteryPercent: Int? = null

    val error = bleManager.error
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), null)

    init {
        viewModelScope.launch {
            bleManager.packets.collect { packet ->
                _latestPacket.value = packet
                if (packet.kneeAngle > _peakKneeAngle.value) {
                    _peakKneeAngle.value = packet.kneeAngle
                }
                updateBatteryEstimate(packet.batteryPercent, packet.timestampMs)
            }
        }
    }

    /**
     * Estimates remaining battery life by tracking drain rate over time.
     * Requires at least 1% drop and 1 minute of data before producing an estimate.
     */
    private fun updateBatteryEstimate(percent: Int, timestampMs: Long) {
        if (firstBatteryPercent == null) {
            firstBatteryPercent = percent
            firstBatteryTimeMs = timestampMs
            lastBatteryPercent = percent
            return
        }

        if (percent == lastBatteryPercent) return
        lastBatteryPercent = percent

        val dropped = (firstBatteryPercent ?: return) - percent
        val elapsedMin = (timestampMs - firstBatteryTimeMs) / 60_000f
        if (dropped < 1 || elapsedMin < 1f) return

        val drainPerMin = dropped / elapsedMin
        _batteryEstimateMin.value = percent / drainPerMin
    }

    fun resetPeakAngle() {
        _peakKneeAngle.value = 0f
    }
}
