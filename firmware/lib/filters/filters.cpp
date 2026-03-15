#include "filters.h"
#include <cmath>

/** @brief Set smoothing factor and reset state. */
void LowPassFilter::configure(float alpha) {
    m_alpha       = alpha;
    m_value       = 0.0f;
    m_initialized = false;
}

/** @brief Apply filter to a raw sample and return smoothed value. */
float LowPassFilter::apply(float raw) {
    if (!m_initialized) {
        m_value       = raw;
        m_initialized = true;
        return m_value;
    }

    m_value = m_alpha * raw + (1.0f - m_alpha) * m_value;

    return m_value;
}

/** @brief Reset filter state. */
void LowPassFilter::reset() {
    m_value       = 0.0f;
    m_initialized = false;
}

/**
 * @brief Fast inverse square root approximation (Quake III).
 *
 * Uses the Quake III "magic number" 0x5f3759df for an initial
 * approximation, then refines with two Newton-Raphson iterations.
 */
static float invSqrt(float x) {
    float y = x;
    long i = *(long*)&y;

    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;
    
    /* Newton-Raphson */
    float halfx = 0.5f * x;
    y = y * (1.5f - (halfx * y * y));
    y = y * (1.5f - (halfx * y * y));
    
    return y;
}

/**
 * @brief Set sample rate and filter gain, reset quaternion to identity.
 * @param [in] sampleFreq Update rate in Hz.
 * @param [in] gain Filter gain (beta). Higher = faster convergence, more noise.
 */
void MadgwickFilter::configure(float sampleFreq, float gain) {
    m_samplePeriod          = 1.0f / sampleFreq;
    m_gain                  = gain;
    m_quaternion.element.w  = 1.0f;
    m_quaternion.element.x  = 0.0f;
    m_quaternion.element.y  = 0.0f;
    m_quaternion.element.z  = 0.0f;
}

/**
 * @brief 6-axis IMU update.
 *
 * Computes the quaternion derivative from gyroscope angular rates,
 * then applies a gradient descent correction step using the
 * accelerometer to compensate for gyro drift.
 *
 * @param [in] gx Gyroscope x (rad/s).
 * @param [in] gy Gyroscope y (rad/s).
 * @param [in] gz Gyroscope z (rad/s).
 * @param [in] ax Accelerometer x (m/s^2).
 * @param [in] ay Accelerometer y (m/s^2).
 * @param [in] az Accelerometer z (m/s^2).
 */
void MadgwickFilter::update(float gx, float gy, float gz,
                            float ax, float ay, float az) {
    float recipNorm;

    /** Shorthand for quaternion components */
    float w = m_quaternion.element.w;
    float x = m_quaternion.element.x;
    float y = m_quaternion.element.y;
    float z = m_quaternion.element.z;

    /**
     * Quaternion rate of change from gyroscope.
     * qDot = 0.5 * q (x) omega, where omega = [0, gx, gy, gz]
     */
    Quaternion qDot;
    qDot.element.w = 0.5f * (-x * gx - y * gy - z * gz);
    qDot.element.x = 0.5f * ( w * gx + y * gz - z * gy);
    qDot.element.y = 0.5f * ( w * gy - x * gz + z * gx);
    qDot.element.z = 0.5f * ( w * gz + x * gy - y * gx);

    /** Apply gradient descent correction if accelerometer data is valid */
    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
        /** Normalize accelerometer measurement */
        recipNorm = invSqrt(ax * ax + ay * ay + az * az);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;

        /** Jacobian terms for gradient descent. */
        float twoW = 2.0f * w;
        float twoX = 2.0f * x;
        float twoY = 2.0f * y;
        float twoZ = 2.0f * z;
        float fourW = 4.0f * w;
        float fourX = 4.0f * x;
        float fourY = 4.0f * y;
        float eightX = 8.0f * x;
        float eightY = 8.0f * y;
        float ww = w * w;
        float xx = x * x;
        float yy = y * y;
        float zz = z * z;

        /**
         * Gradient descent step: gradient = J^T * f
         * where f is the objective function (expected vs measured gravity)
         * and J is the Jacobian of f with respect to quaternion components.
         */
        Quaternion gradient;
        gradient.element.w = fourW * yy + twoY * ax + fourW * xx - twoX * ay;
        gradient.element.x = fourX * zz - twoZ * ax + 4.0f * ww * x - twoW * ay
                           - fourX + eightX * xx + eightX * yy + fourX * az;
        gradient.element.y = 4.0f * ww * y + twoW * ax + fourY * zz - twoZ * ay
                           - fourY + eightY * xx + eightY * yy + fourY * az;
        gradient.element.z = 4.0f * xx * z - twoX * ax + 4.0f * yy * z - twoY * ay;

        /** Normalize gradient */
        recipNorm = invSqrt(gradient.element.w * gradient.element.w
                          + gradient.element.x * gradient.element.x
                          + gradient.element.y * gradient.element.y
                          + gradient.element.z * gradient.element.z);
        gradient.element.w *= recipNorm;
        gradient.element.x *= recipNorm;
        gradient.element.y *= recipNorm;
        gradient.element.z *= recipNorm;

        /** Apply feedback: subtract gain-scaled gradient from gyro rate */
        qDot.element.w -= m_gain * gradient.element.w;
        qDot.element.x -= m_gain * gradient.element.x;
        qDot.element.y -= m_gain * gradient.element.y;
        qDot.element.z -= m_gain * gradient.element.z;
    }

    /** Integrate quaternion rate of change */
    m_quaternion.element.w += qDot.element.w * m_samplePeriod;
    m_quaternion.element.x += qDot.element.x * m_samplePeriod;
    m_quaternion.element.y += qDot.element.y * m_samplePeriod;
    m_quaternion.element.z += qDot.element.z * m_samplePeriod;

    /** Normalize quaternion */
    recipNorm = invSqrt(m_quaternion.element.w * m_quaternion.element.w
                      + m_quaternion.element.x * m_quaternion.element.x
                      + m_quaternion.element.y * m_quaternion.element.y
                      + m_quaternion.element.z * m_quaternion.element.z);
    m_quaternion.element.w *= recipNorm;
    m_quaternion.element.x *= recipNorm;
    m_quaternion.element.y *= recipNorm;
    m_quaternion.element.z *= recipNorm;
}

/**
 * @brief Extract Euler angles (degrees) from current quaternion.
 * @param [out] pPitch Rotation about X-axis.
 * @param [out] pRoll Rotation about Y-axis.
 * @param [out] pYaw Rotation about Z-axis.
 */
void MadgwickFilter::getEuler(float* pPitch, float* pRoll, float* pYaw) const {
    float w = m_quaternion.element.w;
    float x = m_quaternion.element.x;
    float y = m_quaternion.element.y;
    float z = m_quaternion.element.z;

    *pPitch = atan2f(2.0f * (w * x + y * z),
                     1.0f - 2.0f * (x * x + y * y)) * (180.0f / M_PI);

    *pRoll  = asinf(2.0f * (w * y - z * x)) * (180.0f / M_PI);
    
    *pYaw   = atan2f(2.0f * (w * z + x * y),
                     1.0f - 2.0f * (y * y + z * z)) * (180.0f / M_PI);
}

/**
 * @brief Get gravity vector in sensor frame (units of g).
 * @param [out] pGx Gravity x component.
 * @param [out] pGy Gravity y component.
 * @param [out] pGz Gravity z component.
 */
void MadgwickFilter::getGravity(float* pGx, float* pGy, float* pGz) const {
    float w = m_quaternion.element.w;
    float x = m_quaternion.element.x;
    float y = m_quaternion.element.y;
    float z = m_quaternion.element.z;

    *pGx = 2.0f * (x * z - w * y);
    *pGy = 2.0f * (w * x + y * z);
    *pGz = w * w - x * x - y * y + z * z;
}
