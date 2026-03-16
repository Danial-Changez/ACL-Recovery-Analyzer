package com.example.aclrecoveryanalyzer.data.ble

import java.util.UUID

object BleConstants {
    const val DEVICE_NAME = "ACL-Rehab"

    val SERVICE_UUID: UUID = UUID.fromString("13953057-0959-4d32-ab62-362a7ffb6aab")
    val LIVE_DATA_CHAR_UUID: UUID = UUID.fromString("13953057-0959-4d32-ab62-362a7ffb6aac")
    val CCCD_UUID: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

    const val REHAB_PACKET_SIZE = 23
    const val SCAN_TIMEOUT_MS = 15_000L
    const val RECONNECT_DELAY_MS = 3_000L
    const val MAX_RECONNECT_ATTEMPTS = 5
}
