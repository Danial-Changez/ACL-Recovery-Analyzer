#include "battery.h"
#include "config.h"
#include "filters.h"

#include <Arduino.h>
#include <esp_adc_cal.h>

/** Static member definitions */
uint8_t Battery::m_percent = 100;
float   Battery::m_voltage = 4.2f;

/** Internal state */
static LowPassFilter voltFilter;
static esp_adc_cal_characteristics_t adcChars;

/** LiPo discharge curve for 2000mAh 103450 cell at 0.2C, 25°C. */
static const float dischargeLUT[][2] = {
    /* { voltage,  remaining % } */
    {   4.20f,     100.0f  },
    {   4.05f,      90.0f  },
    {   3.95f,      80.0f  },
    {   3.88f,      70.0f  },
    {   3.82f,      60.0f  },
    {   3.78f,      50.0f  },
    {   3.73f,      40.0f  },
    {   3.68f,      30.0f  },
    {   3.58f,      20.0f  },
    {   3.45f,      10.0f  },
    {   3.30f,       4.0f  },
};
static const int LUT_SIZE = sizeof(dischargeLUT) / sizeof(dischargeLUT[0]);

/** @brief Configure ADC resolution and attenuation, take initial reading. */
void Battery::init() {
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_BATTERY_ADC, ADC_11db);

    /** Use per-chip eFuse calibration data for accurate voltage readings */
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adcChars);

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

    /** Use calibrated conversion: returns millivolts at the ADC pin */
    uint32_t adcMv = esp_adc_cal_raw_to_voltage((uint32_t)adcAvg, &adcChars);
    float voltage = (adcMv / 1000.0f) * BATTERY_DIVIDER_RATIO;
    m_voltage = voltFilter.apply(voltage);

    Serial.printf("[Battery] ADC avg: %.1f, pin mV: %lu, battery: %.3fV\n",
                  adcAvg, adcMv, m_voltage);
    
    /** LUT interpolation — walk table to find the two bracketing entries */
    float percent;
    if (m_voltage >= dischargeLUT[0][0]) {
        percent = 100.0f;
    } else if (m_voltage <= dischargeLUT[LUT_SIZE - 1][0]) {
        percent = 0.0f;
    } else {
        for (int i = 0; i < LUT_SIZE - 1; i++) {
            if (m_voltage >= dischargeLUT[i + 1][0]) {
                float vHigh = dischargeLUT[i][0];
                float vLow  = dischargeLUT[i + 1][0];
                float pHigh = dischargeLUT[i][1];
                float pLow  = dischargeLUT[i + 1][1];
                percent = pLow + (pHigh - pLow) * (m_voltage - vLow) / (vHigh - vLow);
                break;
            }
        }
    }
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
