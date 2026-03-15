#ifndef HAPTIC_H
#define HAPTIC_H

#include <cstdint>

#define VIB_LEDC_CHANNEL   0
#define MAX_PATTERN_STEPS  6

/** Vibration motor */
struct VibStep {
    uint32_t durationMs;
    uint8_t  pwmDuty;
};

enum LedMode { 
  LED_OFF, 
  LED_SOLID, 
  LED_BREATHE 
};

/**
 * @brief Haptic feedback via vibration motor and NeoPixel LED.
 *
 * Provides vibration patterns for rep feedback and LED effects
 * for visual status indication.
 */
class Haptic {
  public:
    static void init();
    static void update(uint32_t nowMs);
    static void buzzShort();
    static void buzzDouble();
    static void buzzLong();
    static void ledSetColor(uint8_t r, uint8_t g, uint8_t b);
    static void ledBreathe(uint8_t r, uint8_t g, uint8_t b, uint32_t periodMs);
    static void ledOff();
};

#endif /* HAPTIC_H */
