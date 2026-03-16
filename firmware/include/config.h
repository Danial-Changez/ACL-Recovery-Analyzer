#ifndef CONFIG_H
#define CONFIG_H

/*
 * Single IMU Testing Mode
 *
 * Uncomment to run with only one IMU.
 * Absolute pitch is used instead of relative knee angle.
 * Comment out when both IMUs are wired.
 */

// #define SINGLE_IMU_MODE

/*
 * Pin Assignments
 *
 * Pin mappings to ESP32-S3-Zero IO.
 * Refer to official documentation when needed:
 *  - https://www.espboards.dev/esp32/esp32-s3-zero/
 */

#define PIN_I2C_SCL       5
#define PIN_I2C_SDA       6
#define PIN_VIBRATION     7
#define PIN_LED           21
#define PIN_BATTERY_ADC   8
#define PIN_BUTTON        9

/*
 * IMU I2C Addresses
 *
 * Addresses and clock speed for both MPU-6050 IMUs.
 *
 * AD0 LOW  = 0x68
 * AD0 HIGH = 0x69
 */

#define IMU_THIGH_ADDR    0x69
#define IMU_SHANK_ADDR    0x68
#define I2C_CLOCK_HZ      100000

/*
 * Sampling & Transmission Intervals
 *
 * Controls how frequently sensors are read, BLE packets are
 * sent, and battery voltage is checked.
 */

#define SENSOR_INTERVAL_MS   20
#define BLE_INTERVAL_MS      100
#define BATTERY_INTERVAL_MS  30000

/*
 * Madgwick Filter
 *
 * Orientation filter parameters. Gain controls convergence
 * speed vs. noise rejection. A higher startup gain lets the
 * filter lock on quickly after boot.
 */

#define MADGWICK_GAIN           0.1f
#define MADGWICK_GAIN_STARTUP   0.5f
#define MADGWICK_SAMPLE_FREQ    50.0f

/*
 * Exercise Tracking
 *
 * Angle thresholds that define a valid rep. A rep begins when
 * the knee straightens past START_ANGLE and counts when flexion
 * reaches PEAK_ANGLE.
 */

#define EXERCISE_START_ANGLE    10.0f
#define EXERCISE_PEAK_ANGLE     70.0f
#define ROM_LPF_ALPHA           0.2f

/*
 * Battery
 *
 * Voltage thresholds for a single-cell (2000mAh) LiPo
 * and the external voltage divider (100k + 200k) ratio
 * used on the ADC input.
 *
 * Refer to datasheet when needed:
 *  - https://www.ufinebattery.com/products/3-7-v-2000mah-lithium-ion-battery-103450/
 */

#define BATTERY_FULL_VOLTAGE    4.2f
#define BATTERY_EMPTY_VOLTAGE   3.3f
#define BATTERY_LOW_PERCENT     10
#define BATTERY_DIVIDER_RATIO   1.5f

/*
 * Haptic Feedback
 *
 * Vibration durations for user feedback events.
 */

#define VIBRATE_SHORT_MS     100
#define VIBRATE_LONG_MS      300
#define VIBRATE_PAUSE_MS     150

/*
 * Calibration
 *
 * Startup calibration parameters. The device must remain
 * stationary for CALIB_SETTLE_MS while gyro variance stays
 * below GYRO_STABLE_THRESHOLD.
 */

#define CALIB_SETTLE_MS       3000
#define GYRO_STABLE_THRESHOLD 0.05f
#define IMU_IDLE_TIMEOUT_MS   30000

#endif /* CONFIG_H */
