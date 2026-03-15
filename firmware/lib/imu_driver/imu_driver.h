#ifndef IMU_DRIVER_H
#define IMU_DRIVER_H

#include "types.h"

#define GRAVITY 9.80665f
#define D2R (float)(M_PI / 180.0)

enum class ImuId : uint8_t {
    THIGH,
    SHANK
};

/**
 * @brief IMU driver handling dual MPU-6050 sensors and Madgwick fusion.
 *
 * Manages I2C communication with two MPU-6050 IMUs, runs Madgwick
 * orientation filters, and provides Euler angles, acceleration,
 * and calibration status.
 */
class ImuDriver {
  public:
    static bool        init();
    static void        update();
    static EulerAngles readEuler(ImuId id);
    static Vec3        readRawAccel(ImuId id);
    static Vec3        readLinearAccel(ImuId id);
    static float       computeKneeAngle();
    static CalibStatus getCalibStatus();
    static void        enterSleep(ImuId id);
    static void        wake(ImuId id);
};

#endif /* IMU_DRIVER_H */
