package com.example.aclrecoveryanalyzer.ui.live

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Battery3Bar
import androidx.compose.material.icons.filled.FitnessCenter
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material.icons.filled.Sensors
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.hilt.lifecycle.viewmodel.compose.hiltViewModel
import com.example.aclrecoveryanalyzer.data.ble.ConnectionState

@Composable
fun LiveSessionScreen(
    viewModel: LiveSessionViewModel = hiltViewModel()
) {
    val connectionState by viewModel.connectionState.collectAsState()
    val packet by viewModel.latestPacket.collectAsState()
    val peakAngle by viewModel.peakKneeAngle.collectAsState()
    val batteryEstimate by viewModel.batteryEstimateMin.collectAsState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        // Connection status banner
        ConnectionBanner(connectionState)

        if (connectionState == ConnectionState.CONNECTED && packet != null) {
            val p = packet!!

            // Big knee angle display
            KneeAngleCard(
                currentAngle = p.kneeAngle,
                peakAngle = peakAngle,
                onResetPeak = viewModel::resetPeakAngle
            )

            // Exercise info
            ExerciseCard(
                exerciseType = p.exerciseType.name.replace("_", " "),
                repCount = p.repCount,
                stepCount = p.stepCount,
                isActive = p.flags.exerciseActive
            )

            // Battery & calibration
            StatusCard(
                batteryPercent = p.batteryPercent,
                gyroCalibrated = p.calibStatus.gyroStable,
                accelCalibrated = p.calibStatus.accelStable,
                lowBattery = p.flags.lowBattery,
                estimateMin = batteryEstimate
            )

            // IMU detail card
            ImuDetailCard(
                thighPitch = p.thighPitch,
                thighRoll = p.thighRoll,
                shankPitch = p.shankPitch,
                shankRoll = p.shankRoll,
                linearAccelZ = p.linearAccelZ
            )
        } else {
            // Not connected placeholder
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .weight(1f),
                verticalArrangement = Arrangement.Center,
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Icon(
                    Icons.Default.Sensors,
                    contentDescription = null,
                    modifier = Modifier.size(64.dp),
                    tint = MaterialTheme.colorScheme.onSurfaceVariant
                )
                Spacer(Modifier.height(16.dp))
                Text(
                    text = "No device connected",
                    style = MaterialTheme.typography.titleMedium
                )
                Text(
                    text = "Go to the Device tab to scan and connect",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}

@Composable
private fun ConnectionBanner(state: ConnectionState) {
    val (text, color) = when (state) {
        ConnectionState.CONNECTED -> "Connected" to MaterialTheme.colorScheme.primary
        ConnectionState.CONNECTING -> "Connecting..." to MaterialTheme.colorScheme.tertiary
        ConnectionState.RECONNECTING -> "Reconnecting..." to MaterialTheme.colorScheme.error
        ConnectionState.SCANNING -> "Scanning..." to MaterialTheme.colorScheme.tertiary
        ConnectionState.DISCONNECTED -> "Disconnected" to MaterialTheme.colorScheme.onSurfaceVariant
    }
    Text(
        text = text,
        style = MaterialTheme.typography.labelLarge,
        color = color,
        modifier = Modifier.padding(bottom = 4.dp)
    )
}

@Composable
private fun KneeAngleCard(currentAngle: Float, peakAngle: Float, onResetPeak: () -> Unit) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.primaryContainer
        )
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(20.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = "Knee Angle",
                style = MaterialTheme.typography.titleSmall,
                color = MaterialTheme.colorScheme.onPrimaryContainer
            )
            Text(
                text = "%.1f\u00B0".format(currentAngle),
                fontSize = 56.sp,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.onPrimaryContainer
            )
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    text = "Peak: %.1f\u00B0".format(peakAngle),
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onPrimaryContainer.copy(alpha = 0.7f)
                )
                IconButton(onClick = onResetPeak, modifier = Modifier.size(24.dp)) {
                    Icon(
                        Icons.Default.Refresh,
                        contentDescription = "Reset peak",
                        modifier = Modifier.size(16.dp),
                        tint = MaterialTheme.colorScheme.onPrimaryContainer.copy(alpha = 0.7f)
                    )
                }
            }
        }
    }
}

@Composable
private fun ExerciseCard(exerciseType: String, repCount: Int, stepCount: Int, isActive: Boolean) {
    Card(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(
                horizontalArrangement = Arrangement.spacedBy(12.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(Icons.Default.FitnessCenter, contentDescription = null)
                Column {
                    Text(
                        text = if (isActive) exerciseType else "No Exercise",
                        style = MaterialTheme.typography.titleSmall
                    )
                    Text(
                        text = "Reps: $repCount | Steps: $stepCount",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
            if (isActive) {
                Text(
                    text = "ACTIVE",
                    style = MaterialTheme.typography.labelMedium,
                    color = MaterialTheme.colorScheme.primary
                )
            }
        }
    }
}

@Composable
private fun StatusCard(
    batteryPercent: Int,
    gyroCalibrated: Boolean,
    accelCalibrated: Boolean,
    lowBattery: Boolean,
    estimateMin: Float?
) {
    Card(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Column {
                Row(
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Icon(
                        Icons.Default.Battery3Bar,
                        contentDescription = null,
                        tint = if (lowBattery) MaterialTheme.colorScheme.error
                        else MaterialTheme.colorScheme.onSurface
                    )
                    Text(
                        text = "$batteryPercent%",
                        style = MaterialTheme.typography.titleMedium,
                        color = if (lowBattery) MaterialTheme.colorScheme.error
                        else MaterialTheme.colorScheme.onSurface
                    )
                }
                Text(
                    text = if (estimateMin != null) {
                        val hours = (estimateMin / 60).toInt()
                        val mins = (estimateMin % 60).toInt()
                        if (hours > 0) "~${hours}h ${mins}m remaining"
                        else "~${mins}m remaining"
                    } else {
                        "Estimating..."
                    },
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
            Column(horizontalAlignment = Alignment.End) {
                Text(
                    text = "Gyro: ${if (gyroCalibrated) "OK" else "..."}",
                    style = MaterialTheme.typography.bodySmall
                )
                Text(
                    text = "Accel: ${if (accelCalibrated) "OK" else "..."}",
                    style = MaterialTheme.typography.bodySmall
                )
            }
        }
    }
}

@Composable
private fun ImuDetailCard(
    thighPitch: Float,
    thighRoll: Float,
    shankPitch: Float,
    shankRoll: Float,
    linearAccelZ: Float
) {
    Card(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = "IMU Details",
                style = MaterialTheme.typography.titleSmall,
                modifier = Modifier.padding(bottom = 8.dp)
            )
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Column {
                    Text("Thigh", style = MaterialTheme.typography.labelMedium)
                    Text(
                        "Pitch: %.1f\u00B0".format(thighPitch),
                        style = MaterialTheme.typography.bodySmall
                    )
                    Text(
                        "Roll: %.1f\u00B0".format(thighRoll),
                        style = MaterialTheme.typography.bodySmall
                    )
                }
                Column {
                    Text("Shank", style = MaterialTheme.typography.labelMedium)
                    Text(
                        "Pitch: %.1f\u00B0".format(shankPitch),
                        style = MaterialTheme.typography.bodySmall
                    )
                    Text(
                        "Roll: %.1f\u00B0".format(shankRoll),
                        style = MaterialTheme.typography.bodySmall
                    )
                }
                Column {
                    Text("Accel Z", style = MaterialTheme.typography.labelMedium)
                    Text(
                        "%.2f m/s\u00B2".format(linearAccelZ),
                        style = MaterialTheme.typography.bodySmall
                    )
                }
            }
        }
    }
}
