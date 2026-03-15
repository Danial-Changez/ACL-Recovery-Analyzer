#include <Adafruit_MPU6050.h>
#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "filters.h"
#include "imu_driver.h"

/** Internal state */
static Adafruit_MPU6050 imuThigh;
static Adafruit_MPU6050 imuShank;

static MadgwickFilter filterThigh;
static MadgwickFilter filterShank;

static Vec3        rawAccel[2];
static Vec3        rawGyro[2];
static EulerAngles euler[2];

/** Calibration tracking */
static CalibStatus calibStatus = {false, false};
static uint32_t    initTimeMs  = 0;
static float       gyroVariance[2] = {999.0f, 999.0f};

#define CALIB_WINDOW  50
static float gyroHistory[2][CALIB_WINDOW];
static int   gyroHistIdx[2]  = {0, 0};
static bool  gyroHistFull[2] = {false, false};

static float computeVariance(const float* pBuf, int count) {
    float sum1 = 0.0f;
    float sum2 = 0.0f;

    for (int i = 0; i < count; i++) {
        sum1 += pBuf[i];
        sum2 += pBuf[i] * pBuf[i];
    }
    float mean = sum1 / (float)count;
    return (sum2 / (float)count) - (mean * mean);
}

static bool initSensor(Adafruit_MPU6050& sensor, uint8_t addr, const char* pLabel) {
    if (!sensor.begin(addr, &Wire)) {
        Serial.printf("[IMU] %s (0x%02X) init FAILED\n", pLabel, addr);
        return false;
    }

    sensor.setAccelerometerRange(MPU6050_RANGE_4_G);
    sensor.setGyroRange(MPU6050_RANGE_500_DEG);
    sensor.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.printf("[IMU] %s (0x%02X) OK\n", pLabel, addr);
    return true;
}

/**
 * @brief Initialize both IMUs and Madgwick filters.
 * @return false if either sensor fails to initialize.
 */
bool ImuDriver::init() {
    if (!initSensor(imuThigh, IMU_THIGH_ADDR, "Thigh")) {
        return false;
    }

    if (!initSensor(imuShank, IMU_SHANK_ADDR, "Shank")) {
        return false;
    }

    filterThigh.configure(MADGWICK_SAMPLE_FREQ, MADGWICK_GAIN_STARTUP);
    filterShank.configure(MADGWICK_SAMPLE_FREQ, MADGWICK_GAIN_STARTUP);

    initTimeMs = millis();

    for (int i = 0; i < 2; i++) {
        rawAccel[i]     = {0, 0, 0};
        rawGyro[i]      = {0, 0, 0};
        euler[i]        = {0, 0, 0};
        gyroHistIdx[i]  = 0;
        gyroHistFull[i] = false;
    }

    Serial.println("[IMU] Dual MPU-6050 + Madgwick initialized");
    return true;
}

/** @brief Read both sensors and run Madgwick fusion. Call at 50 Hz. */
void ImuDriver::update() {
    sensors_event_t a, g, temp;

    /** Read thigh IMU */
    if (imuThigh.getEvent(&a, &g, &temp)) {
        rawAccel[0] = {a.acceleration.x, a.acceleration.y, a.acceleration.z};
        rawGyro[0]  = {g.gyro.x, g.gyro.y, g.gyro.z};

        filterThigh.update(rawGyro[0].m_x, rawGyro[0].m_y, rawGyro[0].m_z,
                              rawAccel[0].m_x, rawAccel[0].m_y, rawAccel[0].m_z);

        filterThigh.getEuler(&euler[0].m_pitch, &euler[0].m_roll, &euler[0].m_heading);

        float gMag = sqrtf(rawGyro[0].m_x * rawGyro[0].m_x +
                           rawGyro[0].m_y * rawGyro[0].m_y +
                           rawGyro[0].m_z * rawGyro[0].m_z);
        gyroHistory[0][gyroHistIdx[0]] = gMag;
        gyroHistIdx[0] = (gyroHistIdx[0] + 1) % CALIB_WINDOW;
        if (gyroHistIdx[0] == 0) {
            gyroHistFull[0] = true;
        }
    }

    /** Read shank IMU */
    if (imuShank.getEvent(&a, &g, &temp)) {
        rawAccel[1] = {a.acceleration.x, a.acceleration.y, a.acceleration.z};
        rawGyro[1]  = {g.gyro.x, g.gyro.y, g.gyro.z};

        filterShank.update(rawGyro[1].m_x, rawGyro[1].m_y, rawGyro[1].m_z,
                              rawAccel[1].m_x, rawAccel[1].m_y, rawAccel[1].m_z);

        filterShank.getEuler(&euler[1].m_pitch, &euler[1].m_roll, &euler[1].m_heading);

        float gMag = sqrtf(rawGyro[1].m_x * rawGyro[1].m_x +
                           rawGyro[1].m_y * rawGyro[1].m_y +
                           rawGyro[1].m_z * rawGyro[1].m_z);
        gyroHistory[1][gyroHistIdx[1]] = gMag;
        gyroHistIdx[1] = (gyroHistIdx[1] + 1) % CALIB_WINDOW;
        if (gyroHistIdx[1] == 0) {
            gyroHistFull[1] = true;
        }
    }

    /** Reduce filter gain after startup period */
    if (millis() - initTimeMs > CALIB_SETTLE_MS) {
        filterThigh.m_gain = MADGWICK_GAIN;
        filterShank.m_gain = MADGWICK_GAIN;
    }

    /** Update calibration status */
    if (gyroHistFull[0]) {
        gyroVariance[0] = computeVariance(gyroHistory[0], CALIB_WINDOW);
    }
    if (gyroHistFull[1]) {
        gyroVariance[1] = computeVariance(gyroHistory[1], CALIB_WINDOW);
    }

    calibStatus.m_gyroStable  = (gyroVariance[0] < GYRO_STABLE_THRESHOLD) &&
                                (gyroVariance[1] < GYRO_STABLE_THRESHOLD);
    calibStatus.m_accelStable = calibStatus.m_gyroStable;
}

/** @brief Get latest Euler angles (degrees) for specified sensor. */
EulerAngles ImuDriver::readEuler(ImuId id) {
    return euler[(uint8_t)id];
}

/** @brief Raw accelerometer in m/s2 (gravity included). */
Vec3 ImuDriver::readRawAccel(ImuId id) {
    return rawAccel[(uint8_t)id];
}

/** @brief Accelerometer with gravity subtracted using quaternion. */
Vec3 ImuDriver::readLinearAccel(ImuId id) {
    uint8_t idx = (uint8_t)id;
    float   gx, gy, gz;

    if (idx == 0) {
        filterThigh.getGravity(&gx, &gy, &gz);
    } else {
        filterShank.getGravity(&gx, &gy, &gz);
    }

    return {
        rawAccel[idx].m_x - gx * GRAVITY,
        rawAccel[idx].m_y - gy * GRAVITY,
        rawAccel[idx].m_z - gz * GRAVITY
    };
}

/** @brief Relative knee angle: shank.pitch - thigh.pitch (degrees). */
float ImuDriver::computeKneeAngle() {
    return euler[1].m_pitch - euler[0].m_pitch;
}

/** @brief Calibration status -- have sensors settled? */
CalibStatus ImuDriver::getCalibStatus() {
    return calibStatus;
}

/** @brief Enter low-power mode for specified sensor. */
void ImuDriver::enterSleep(ImuId id) {
    if (id == ImuId::THIGH) {
        imuThigh.enableSleep(true);
    } else {
        imuShank.enableSleep(true);
    }
}

/** @brief Wake specified sensor from low-power mode. */
void ImuDriver::wake(ImuId id) {
    if (id == ImuId::THIGH) {
        imuThigh.enableSleep(false);
    } else {
        imuShank.enableSleep(false);
    }
}
