#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "types.h"
#include "imu_driver.h"
#include "haptic.h"
#include "battery.h"
#include "ble_service.h"
#include "exercise_tracker.h"

static uint32_t lastSensorMs  = 0;
static uint32_t lastBleMs     = 0;
static uint32_t lastBatteryMs = 0;

static bool     lastButtonState = HIGH;
static uint32_t lastButtonMs    = 0;
static uint32_t lastActivityMs  = 0;
static bool     imuSleeping     = false;
#define DEBOUNCE_MS  200

void setup() {
    setCpuFrequencyMhz(80);
    Serial.begin(115200);
    delay(1000);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(I2C_CLOCK_HZ);
    delay(500);

    if (!ImuDriver::init()) {
        Serial.println("IMU init failed");
        while (true) { delay(1000); }
    }

    Haptic::init();
    Battery::init();
    BleService::init();
    ExerciseTracker::init();

    pinMode(PIN_BUTTON, INPUT_PULLUP);

    Serial.printf("[Main] Exercise: %s\n", ExerciseTracker::getProfile().m_pName);
    Serial.println("[Main] All subsystems initialized");
}

void loop() {
    uint32_t now = millis();

    /** Button press cycles exercise type */
    bool buttonState = digitalRead(PIN_BUTTON);
    if (buttonState == LOW && lastButtonState == HIGH &&
        (now - lastButtonMs) > DEBOUNCE_MS) {
        lastButtonMs   = now;
        lastActivityMs = now;

        if (imuSleeping) {
            ImuDriver::wake(ImuId::THIGH);
            ImuDriver::wake(ImuId::SHANK);
            imuSleeping = false;
            Serial.println("[Main] IMUs woken (button press)");
        }

        ExerciseTracker::nextExercise();
        Haptic::buzzDouble();
        Serial.printf("[Main] Exercise: %s\n", ExerciseTracker::getProfile().m_pName);
    }
    lastButtonState = buttonState;

    /** Sensor read + exercise update at SENSOR_INTERVAL_MS (20ms / 50Hz) */
    if (now - lastSensorMs >= SENSOR_INTERVAL_MS) {
        lastSensorMs = now;

        if (!imuSleeping) {
            ImuDriver::update();

            float       kneeAngle   = ImuDriver::computeKneeAngle();
            EulerAngles thighAngles = ImuDriver::readEuler(ImuId::THIGH);

            ExerciseTracker::update(kneeAngle, thighAngles.m_roll, thighAngles.m_pitch, now);
            ExerciseMetrics ex = ExerciseTracker::getMetrics();

            if (ex.m_exerciseActive)
                lastActivityMs = now;
            if (ex.m_repJustCompleted)
                Haptic::buzzShort();

            /** Sleep IMUs after inactivity timeout */
            if ((now - lastActivityMs) > IMU_IDLE_TIMEOUT_MS) {
                ImuDriver::enterSleep(ImuId::THIGH);
                ImuDriver::enterSleep(ImuId::SHANK);
                imuSleeping = true;
                Serial.println("[Main] IMUs sleeping (idle timeout)");
            }
        }
    }

    /** BLE transmission at BLE_INTERVAL_MS (100ms / 10Hz) */
    if (now - lastBleMs >= BLE_INTERVAL_MS) {
        lastBleMs = now;

        if (BleService::isConnected()) {
            float       kneeAngle   = ImuDriver::computeKneeAngle();
            EulerAngles thighAngles = ImuDriver::readEuler(ImuId::THIGH);
            EulerAngles shankAngles = ImuDriver::readEuler(ImuId::SHANK);
            Vec3        linAccel    = ImuDriver::readLinearAccel(ImuId::SHANK);
            ExerciseMetrics ex      = ExerciseTracker::getMetrics();
            CalibStatus     calib   = ImuDriver::getCalibStatus();

            RehabPacket packet;
            packet.kneeAngle    = (int16_t)(kneeAngle * 100.0f);
            packet.thighPitch   = (int16_t)(thighAngles.m_pitch * 100.0f);
            packet.thighRoll    = (int16_t)(thighAngles.m_roll * 100.0f);
            packet.thighHeading = (int16_t)(thighAngles.m_heading * 100.0f);
            packet.shankPitch   = (int16_t)(shankAngles.m_pitch * 100.0f);
            packet.shankRoll    = (int16_t)(shankAngles.m_roll * 100.0f);
            packet.shankHeading = (int16_t)(shankAngles.m_heading * 100.0f);
            packet.linearAccelZ = (int16_t)(linAccel.m_z * 100.0f);
            packet.stepCount    = 0;
            packet.repCount     = ex.m_repCount;
            packet.exerciseType = static_cast<uint8_t>(ExerciseTracker::getExerciseType());
            packet.calibStatus  = (calib.m_gyroStable ? 0x01 : 0x00) |
                                  (calib.m_accelStable ? 0x02 : 0x00);
            packet.batteryPercent = Battery::getPercent();
            packet.flags          = (ex.m_exerciseActive ? 0x01 : 0x00) |
                                    (Battery::isLow() ? 0x04 : 0x00) |
                                    (!calib.m_gyroStable ? 0x08 : 0x00);

            BleService::sendPacket(packet);
        }
    }

    /** Battery check at BATTERY_INTERVAL_MS (30s) */
    if (now - lastBatteryMs >= BATTERY_INTERVAL_MS) {
        lastBatteryMs = now;
        Battery::update();

        if (Battery::isLow()) {
            Haptic::buzzLong();
        }
    }

    /** Haptic pattern engine + LED update */
    Haptic::update(now);
}
