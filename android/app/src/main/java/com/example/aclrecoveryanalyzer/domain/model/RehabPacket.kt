package com.example.aclrecoveryanalyzer.domain.model

data class RehabPacket(
    val kneeAngle: Float,
    val thighPitch: Float,
    val thighRoll: Float,
    val thighHeading: Float,
    val shankPitch: Float,
    val shankRoll: Float,
    val shankHeading: Float,
    val linearAccelZ: Float,
    val stepCount: Int,
    val repCount: Int,
    val exerciseType: ExerciseType,
    val calibStatus: CalibrationStatus,
    val batteryPercent: Int,
    val flags: PacketFlags,
    val timestampMs: Long = System.currentTimeMillis()
)

data class CalibrationStatus(
    val gyroStable: Boolean,
    val accelStable: Boolean
) {
    companion object {
        fun fromByte(byte: Int): CalibrationStatus = CalibrationStatus(
            gyroStable = (byte and 0x01) != 0,
            accelStable = (byte and 0x02) != 0
        )
    }
}

data class PacketFlags(
    val exerciseActive: Boolean,
    val lowBattery: Boolean,
    val calibIncomplete: Boolean
) {
    companion object {
        fun fromByte(byte: Int): PacketFlags = PacketFlags(
            exerciseActive = (byte and 0x01) != 0,
            lowBattery = (byte and 0x04) != 0,
            calibIncomplete = (byte and 0x08) != 0
        )
    }
}
