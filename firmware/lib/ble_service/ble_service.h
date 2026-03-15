#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include "types.h"

/**
 * @brief BLE GATT server, advertising, and connection state.
 *
 * Manages a NimBLE GATT server with characteristics for live
 * sensor data (notify), session summary (read), and device info (read).
 */
class BleService {
  public:
    static void init();
    static void sendPacket(const RehabPacket& packet);
    static bool isConnected();
};

#endif /* BLE_SERVICE_H */
