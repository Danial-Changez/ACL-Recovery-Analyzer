#include "ble_service.h"
#include "config.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

/** UUIDs */
#define SERVICE_UUID     "13953057-0959-4d32-ab62-362a7ffb6aab"
#define LIVE_DATA_UUID   "13953057-0959-4d32-ab62-362a7ffb6aac"
#define SESSION_UUID     "13953057-0959-4d32-ab62-362a7ffb6aad"
#define DEVICE_INFO_UUID "13953057-0959-4d32-ab62-362a7ffb6aae"

/** Internal state */
static NimBLEServer*         pServer     = nullptr;
static NimBLECharacteristic* pLiveData   = nullptr;
static NimBLECharacteristic* pSessionSum = nullptr;
static NimBLECharacteristic* pDeviceInfo = nullptr;

static const char*   fwVersion    = "ACL-Rehab v0.1.0";
static const uint8_t emptySession[] = {0x00};
static bool          connected   = false;

/** Callback handler for server connection events */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        connected = true;
        Serial.println("===== [NimBLE] Client connected =====");
        Serial.printf(" Address: %s\n", connInfo.getAddress().toString().c_str());
        Serial.printf(" MTU: %d bytes\n", connInfo.getMTU());
        Serial.printf(" Connection Interval: %d /1.25ms\n", connInfo.getConnInterval());
        Serial.printf(" Connection Latency: %d /1.25ms\n", connInfo.getConnLatency());
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        connected = false;
        Serial.println("===== [NimBLE] Client disconnected =====");
        Serial.printf(" Address: %s\n", connInfo.getAddress().toString().c_str());
        Serial.printf(" Reason: %d\n", reason);

        /** Restart advertising */
        pServer->getAdvertising()->start();
    }
} serverCallbacks;

/** @brief Start GATT server and begin advertising. */
void BleService::init() {
    NimBLEDevice::init("ACL-Rehab");

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);
    NimBLEService* pService = pServer->createService(NimBLEUUID(SERVICE_UUID));

    pLiveData = pService->createCharacteristic(
        NimBLEUUID(LIVE_DATA_UUID),
        NIMBLE_PROPERTY::NOTIFY);
    pLiveData->addDescriptor(new NimBLE2904());

    pSessionSum = pService->createCharacteristic(
        NimBLEUUID(SESSION_UUID),
        NIMBLE_PROPERTY::READ);
    pSessionSum->setValue(emptySession, 1);

    pDeviceInfo = pService->createCharacteristic(
        NimBLEUUID(DEVICE_INFO_UUID),
        NIMBLE_PROPERTY::READ);
    pDeviceInfo->setValue((uint8_t*)fwVersion, strlen(fwVersion));

    pService->start();

    /** Setup server advertising */
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName("ACL-Rehab");
    pAdvertising->addServiceUUID(NimBLEUUID(SERVICE_UUID));
    pAdvertising->setMinInterval(800);
    pAdvertising->setMaxInterval(1600);
    NimBLEDevice::startAdvertising();

    Serial.println("[NimBLE] GATT server started, advertising as ACL-Rehab");
}

/**
 * @brief Notify connected client with a sensor packet.
 * @param [in] packet Reference to the RehabPacket to transmit.
 */
void BleService::sendPacket(const RehabPacket& packet) {
    if (!connected || pLiveData == nullptr) {
        return;
    }

    pLiveData->setValue((uint8_t*)&packet, sizeof(RehabPacket));
    pLiveData->notify();
}

/**
 * @brief Check BLE connection status.
 * @return true while a client is connected.
 */
bool BleService::isConnected() {
    return connected;
}
