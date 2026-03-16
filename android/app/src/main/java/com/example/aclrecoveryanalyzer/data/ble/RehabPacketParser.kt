package com.example.aclrecoveryanalyzer.data.ble

import com.example.aclrecoveryanalyzer.domain.model.CalibrationStatus
import com.example.aclrecoveryanalyzer.domain.model.ExerciseType
import com.example.aclrecoveryanalyzer.domain.model.PacketFlags
import com.example.aclrecoveryanalyzer.domain.model.RehabPacket
import java.nio.ByteBuffer
import java.nio.ByteOrder
import javax.inject.Inject
import javax.inject.Singleton

@Singleton
class RehabPacketParser @Inject constructor() {

    fun parse(bytes: ByteArray): RehabPacket? {
        if (bytes.size != BleConstants.REHAB_PACKET_SIZE) return null

        val buf = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)

        return RehabPacket(
            kneeAngle = buf.getShort(0).toFloat() / 100f,
            thighPitch = buf.getShort(2).toFloat() / 100f,
            thighRoll = buf.getShort(4).toFloat() / 100f,
            thighHeading = buf.getShort(6).toFloat() / 100f,
            shankPitch = buf.getShort(8).toFloat() / 100f,
            shankRoll = buf.getShort(10).toFloat() / 100f,
            shankHeading = buf.getShort(12).toFloat() / 100f,
            linearAccelZ = buf.getShort(14).toFloat() / 100f,
            stepCount = buf.getShort(16).toInt() and 0xFFFF,
            repCount = buf.get(18).toInt() and 0xFF,
            exerciseType = ExerciseType.fromValue(buf.get(19).toInt() and 0xFF),
            calibStatus = CalibrationStatus.fromByte(buf.get(20).toInt() and 0xFF),
            batteryPercent = buf.get(21).toInt() and 0xFF,
            flags = PacketFlags.fromByte(buf.get(22).toInt() and 0xFF)
        )
    }
}
