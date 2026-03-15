#include "battery.h"
#include "config.h"
#include "filters.h"

#include <Arduino.h>

/** Static member definitions */
uint8_t Battery::m_percent = 100;
float   Battery::m_voltage = 4.2f;

/** Internal state */
static LowPassFilter voltFilter;

/** @brief Configure ADC resolution and attenuation, take initial reading. */
void Battery::init() {
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_BATTERY_ADC, ADC_11db);

    /** Smoothing to avoid jitter */
    voltFilter.configure(0.1f);
    update();
}

/** @brief Sample ADC, filter voltage, update percent estimate. 
 * 
 * Averages 8 samples to reduce ADC noise.
*/
void Battery::update() {
    uint32_t sum = 0;

    for (int i = 0; i < 8; i++) {
        sum += analogRead(PIN_BATTERY_ADC);
    }

    float adcAvg = (float)sum / 8.0f;
    float voltage = (adcAvg / 4095.0f) * 3.3f * BATTERY_DIVIDER_RATIO;
    m_voltage = voltFilter.apply(voltage);

    Serial.printf("[Battery] ADC avg: %.1f, raw voltage: %.3fV, filtered: %.3fV\n",
                  adcAvg, voltage, m_voltage);
    
    float percent = (m_voltage - BATTERY_EMPTY_VOLTAGE)
        / (BATTERY_FULL_VOLTAGE - BATTERY_EMPTY_VOLTAGE)
        * 100.0f;

    if (percent < 0.0f)
        percent = 0.0f;
    if (percent > 100.0f)
        percent = 100.0f;
    m_percent = (uint8_t)percent;
}

/**
 * @brief Get battery charge percentage.
 * @return Estimated charge level (0-100).
 */
uint8_t Battery::getPercent() {
    return m_percent;
}

/**
 * @brief Check if battery is critically low.
 * @return true if percent <= BATTERY_LOW_PERCENT (10%).
 */
bool Battery::isLow() {
    return m_percent <= BATTERY_LOW_PERCENT;
}

/**
 * @brief Get filtered battery voltage.
 * @return Voltage in volts.
 */
float Battery::getVoltage() {
    return m_voltage;
}
