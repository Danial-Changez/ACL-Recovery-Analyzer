#ifndef BATTERY_H
#define BATTERY_H

#include <cstdint>


/**
 * @brief Battery voltage monitoring via ADC with voltage divider.
 */
class Battery {
  public:
    static void    init();
    static void    update();
    static uint8_t getPercent();
    static bool    isLow();
    static float   getVoltage();

  private:
    static uint8_t m_percent;
    static float   m_voltage;
};

#endif /* BATTERY_H */
