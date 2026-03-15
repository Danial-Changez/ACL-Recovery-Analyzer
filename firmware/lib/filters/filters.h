#ifndef FILTERS_H
#define FILTERS_H

#include <cmath>

/**
 * @brief First-order IIR low-pass filter.
 *
 * Implements y[n] = αx[n] + (1 - α)y[n-1].
 */
class LowPassFilter {
  public:
    void  configure(float alpha);
    float apply(float raw);
    void  reset();

  private:
    float m_alpha       = 0.0f;
    float m_value       = 0.0f;
    bool  m_initialized = false;
};

/**
 * @brief Quaternion orientation representation.
 */
union Quaternion {
    float array[4];
    struct {
        float w, x, y, z;
    } element;
};

/**
 * @brief Madgwick AHRS orientation filter.
 *
 * Fuses gyroscope and accelerometer data into a unit quaternion
 * using the gradient descent correction from the 2011 Madgwick paper.
 * 
 * Refer AHRS documentation:
 *  - https://ahrs.readthedocs.io/en/latest/filters/madgwick.html
 */
class MadgwickFilter {
  public:
    void configure(float sampleFreq, float gain);
    void update(float gx, float gy, float gz, float ax, float ay, float az);
    void getEuler(float* pPitch, float* pRoll, float* pYaw) const;
    void getGravity(float* pGx, float* pGy, float* pGz) const;

    float m_gain = 0.0f;

  private:
    Quaternion m_quaternion   = {1.0f, 0.0f, 0.0f, 0.0f};
    float      m_samplePeriod = 0.02f;
};

#endif /* FILTERS_H */
