#include "haptic.h"
#include "config.h"

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

static VibStep  vibPattern[MAX_PATTERN_STEPS];
static Adafruit_NeoPixel pixel(1, PIN_LED, NEO_GRB + NEO_KHZ800);

static int      vibStepCount   = 0;
static int      vibCurrentStep = -1;
static uint32_t vibStepStartMs = 0;

static LedMode ledMode = LED_OFF;
static uint8_t  ledR = 0, ledG = 0, ledB = 0;
static uint32_t ledPeriodMs = 2000;

static void loadPattern(const VibStep* pSteps, int count) {
    for (int i = 0; i < count && i < MAX_PATTERN_STEPS; i++) {
        vibPattern[i] = pSteps[i];
    }

    vibStepCount   = count;
    vibCurrentStep = 0;
    vibStepStartMs = millis();
    ledcWrite(VIB_LEDC_CHANNEL, vibPattern[0].pwmDuty);
}

/** @brief Initialize vibration motor PWM and NeoPixel LED. */
void Haptic::init() {
    ledcSetup(VIB_LEDC_CHANNEL, 5000, 8);
    ledcAttachPin(PIN_VIBRATION, VIB_LEDC_CHANNEL);
    ledcWrite(VIB_LEDC_CHANNEL, 0);

    pixel.begin();
    pixel.setBrightness(40);
    pixel.clear();
    pixel.show();
}

/** @brief Update vibration pattern engine and LED effects. */
void Haptic::update(uint32_t nowMs) {
    /** Vibration pattern */
    if (vibCurrentStep >= 0 && vibCurrentStep < vibStepCount) {
        uint32_t elapsed = nowMs - vibStepStartMs;

        if (elapsed >= vibPattern[vibCurrentStep].durationMs) {
            vibCurrentStep++;

            if (vibCurrentStep < vibStepCount) {
                vibStepStartMs = nowMs;
                ledcWrite(VIB_LEDC_CHANNEL, vibPattern[vibCurrentStep].pwmDuty);
            } else {
                ledcWrite(VIB_LEDC_CHANNEL, 0);
                vibCurrentStep = -1;
            }
        }
    }

    /** LED update */
    if (ledMode == LED_BREATHE) {
        float phase = (float)(nowMs % ledPeriodMs) / (float)ledPeriodMs;
        float brightness = (sinf(phase * 2.0f * M_PI - M_PI / 2.0f) + 1.0f) / 2.0f;
        uint8_t b = (uint8_t)(brightness * 255.0f);
        pixel.setPixelColor(0, pixel.Color(
            (uint8_t)((ledR * b) >> 8),
            (uint8_t)((ledG * b) >> 8),
            (uint8_t)((ledB * b) >> 8)
        ));
        pixel.show();
    }
}

/** @brief Single 100ms pulse -- rep completed. */
void Haptic::buzzShort() {
    static const VibStep pattern[] = {
        {VIBRATE_SHORT_MS, 200},
        {0, 0}
    };
    loadPattern(pattern, 2);
}

/** @brief Two pulses with gap -- ROM limit alert. */
void Haptic::buzzDouble() {
    static const VibStep pattern[] = {
        {VIBRATE_SHORT_MS, 200},
        {VIBRATE_PAUSE_MS, 0},
        {VIBRATE_SHORT_MS, 200},
        {0, 0}
    };
    loadPattern(pattern, 4);
}

/** @brief 300ms sustained -- session reminder / low battery. */
void Haptic::buzzLong() {
    static const VibStep pattern[] = {
        {VIBRATE_LONG_MS, 200},
        {0, 0}
    };
    loadPattern(pattern, 2);
}

/**
 * @brief Set LED to a solid color.
 * @param [in] r Red channel (0-255).
 * @param [in] g Green channel (0-255).
 * @param [in] b Blue channel (0-255).
 */
void Haptic::ledSetColor(uint8_t r, uint8_t g, uint8_t b) {
    ledMode = LED_SOLID;
    ledR = r; ledG = g; ledB = b;
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();
}

/**
 * @brief Set LED to a breathing pattern.
 * @param [in] r Red channel (0-255).
 * @param [in] g Green channel (0-255).
 * @param [in] b Blue channel (0-255).
 * @param [in] periodMs Full breath cycle duration.
 */
void Haptic::ledBreathe(uint8_t r, uint8_t g, uint8_t b, uint32_t periodMs) {
    ledMode     = LED_BREATHE;
    ledR        = r;
    ledG        = g;
    ledB        = b;
    ledPeriodMs = periodMs;
}

/** @brief Turn off LED. */
void Haptic::ledOff() {
    ledMode = LED_OFF;
    pixel.clear();
    pixel.show();
}
