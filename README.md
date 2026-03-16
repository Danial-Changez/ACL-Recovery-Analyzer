<h1> ACL Recovery Analyzer </h1>

### ENGG*3100 — Group 15

<h2> Table of Contents </h2>

- [Overview](#overview)
  - [Hardware](#hardware)
  - [System Architecture](#system-architecture)
- [Firmware](#firmware)
  - [Sensor Fusion](#sensor-fusion)
  - [BLE Protocol](#ble-protocol)
  - [Exercise Tracking](#exercise-tracking)
  - [Power Management](#power-management)
- [Android App](#android-app)
  - [Architecture](#architecture)
  - [Screens](#screens)
- [Project Structure](#project-structure)
- [Installation](#installation)
  - [Firmware Setup](#firmware-setup)
  - [Android Setup](#android-setup)
- [Usage](#usage)
  - [Device Pairing](#device-pairing)
  - [Exercises](#exercises)
- [Documentation](#documentation)

## Overview

A wearable knee rehabilitation monitoring system for ACL recovery. The device uses dual IMUs strapped above and below the knee to compute live joint angles, transmitting data over BLE. The data is translated to a friendly format in an Android app that displays live metrics, tracks reps, and records session history.

### Hardware

| Component | Part            | Purpose |
|-----------|-----------------|------------------------------------|
| MCU       | ESP32-S3        | Sensor fusion, BLE transmission    |
| IMU (x2)  | MPU-6050        | 6-axis accelerometer and gyroscope |
| Battery   | 2000 mAh LiPo   | ~16.4 hours runtime                |
| Charger   | TP4056          | USB-C LiPo charging                |
| LDO       | ME6217C33       | 3.3V regulation                    |
| Feedback  | Vibration motor | Rep haptic feedback                |

**Sensor Placement:**

```
         ┌──────────┐
         │  Thigh   │  MPU-6050 @ 0x69 (AD0 HIGH)
         │  Sensor  │
         └────┬─────┘
              │
         ═══ KNEE ═══
              │
         ┌────┴─────┐
         │  Shank   │  MPU-6050 @ 0x68 (AD0 LOW)
         │  Sensor  │
         └──────────┘
```

### System Architecture

```
MPU-6050 (Thigh) ──┐                                ┌──────────────────────┐
                   ├─── I2C ──→ ESP32-S3 ── BLE ──→ │  Android App         │
MPU-6050 (Shank) ──┘           │                    │  ├─ Live tab         │
                               ├─ Madgwick filter   │  ├─ History tab      │
                               ├─ Rep counting      │  ├─ Device tab       │
                               ├─ Battery ADC       │  └─ Settings tab     │
                               └─ Haptic feedback   └──────────────────────┘
```

## Firmware

Built with PlatformIO on the Arduino framework for ESP32-S3. Refer [`firmware/`](firmware/) for implementation details.

### Sensor Fusion

Orientation is estimated using a **Madgwick filter** that fuses gyroscope angular rates with accelerometer gravity references through gradient descent. The filter runs at 50 Hz per sensor and outputs quaternions, which are converted to Euler angles.

Knee flexion angle is computed as:

```math
θ_{knee} = θ_{shank\_pitch} − θ_{thigh\_pitch}
```

Both sensors share a single I2C bus at 100 kHz.

### BLE Protocol

The device advertises as `ACL-Rehab` and exposes a custom GATT service with three characteristics:

| Characteristic | UUID Suffix | Property | Description                  |
|----------------|-------------|----------|------------------------------|
| Live Data      | `...6aac`   | NOTIFY   | 23-byte RehabPacket at 10 Hz |
| Session Summary| `...6aad`   | READ     | Aggregated session metrics   |
| Device Info    | `...6aae`   | READ     | Firmware version string      |

**RehabPacket (23 bytes, little-endian):**

```
Offset  Size  Type     Field           Scale
0-1     2     int16    kneeAngle       ÷100
2-3     2     int16    thighPitch      ÷100
4-5     2     int16    thighRoll       ÷100
6-7     2     int16    thighHeading    ÷100
8-9     2     int16    shankPitch      ÷100
10-11   2     int16    shankRoll       ÷100
12-13   2     int16    shankHeading    ÷100
14-15   2     int16    linearAccelZ    ÷100
16-17   2     uint16   stepCount       raw
18      1     uint8    repCount        raw
19      1     uint8    exerciseType    0-4
20      1     uint8    calibStatus     bitmap
21      1     uint8    batteryPercent  0-100
22      1     uint8    flags           bitmap
```

Data throughput: 23 bytes x 10 Hz x 8 bits = 1,840 bps < 1 Mbps PHY limit.

**Note:** Reference silicon labs [Documentaton](https://docs.silabs.com/bluetooth/6.2.0/bluetooth-fundamentals-system-performance/throughput) for more details.

### Exercise Tracking

Five rehabilitation exercises are currently supported, each with configurable angle thresholds and speed limits:

| # | Exercise              | Stage | Angle Source | Target |
|---|-----------------------|-------|--------------|--------|
| 0 | Seated Knee Extension | 1     | Knee angle   | 50°    |
| 1 | Hip Abduction         | 1     | Thigh roll   | 25°    |
| 2 | Single-Leg Squat      | 2     | Knee angle   | 35°    |
| 3 | Leg Bridging          | 3     | Knee angle   | 80°    |
| 4 | Reverse Lunge         | 4     | Knee angle   | 80°    |

Reps are counted by a state machine that tracks angle progression through phases. The order goes

```plain
Start --> Peak --> Return
```

The exercise type is cycled via the onboard button with 200 ms debounce.

### Power Management

| Metric                       | Value       |
|------------------------------|-------------|
| Idle current (measured)      | 146 mA      |
| Optimized current (measured) | 122 mA      |
| Battery capacity             | 2000 mAh    |
| Battery life (optimized)     | ~16.4 hours |

Optimizations applied:
- CPU clock reduced from 240 MHz to 80 MHz.
- BLE advertisement interval increased from 100 ms to a range approximating 1000 ms.
- IMU sleep mode on 30s inactivity.

Battery percentage is estimated using a LUT derived from the cell's actual discharge curve (0.2C rate), replacing the default linear voltage mapping. 

**Note:** Current battery values do not accurately reflect correct battery percentage (WIP).

## Android App

Built with Kotlin, Jetpack Compose, and Material 3. Refer [`android/`](android/).

### Architecture

MVVM + Clean Architecture with three layers:

```
┌─────────────────────────────────────────────────┐
│  Presentation (ui/)                             │
│  ├─ Screens (Composables)                       │
│  └─ ViewModels (StateFlow)                      │
├─────────────────────────────────────────────────┤
│  Domain (domain/)                               │
│  └─ Models (RehabPacket, ExerciseType, etc.)    │
├─────────────────────────────────────────────────┤
│  Data (data/)                                   │
│  └─ BLE (BleManager, RehabPacketParser)         │
└─────────────────────────────────────────────────┘
```

| Technology         | Purpose                                       |
|--------------------|-----------------------------------------------|
| Jetpack Compose    | Declarative UI                                |
| Hilt               | Dependency injection                          |
| Room               | Local session persistence                     |
| Navigation Compose | Type-safe routing with kotlinx.serialization  |
| Vico               | Chart rendering                               |
| DataStore          | User preferences                              |

No internet permission is requested in accordance to PIPEDA compliance. All data stays on-device.

### Screens

| Tab          | Description                                               |
|--------------|-----------------------------------------------------------|
| **Live**     | Live knee angle, rep count, battery %, calibration status |
| **History**  | Past session recordings (placeholder)                     |
| **Device**   | BLE scan, connect/disconnect, device info                 |
| **Settings** | App preferences (placeholder)                             |

## Project Structure

```
acl_recovery_analyzer/
├── firmware/
│   ├── src/main.cpp                    Main loop (sensor read → BLE transmit)
│   ├── include/
│   │   ├── config.h                    Pin assignments, intervals, thresholds
│   │   └── types.h                     RehabPacket struct, enums
│   ├── lib/
│   │   ├── filters/                    Madgwick filter, IIR low-pass
│   │   ├── imu_driver/                 Dual MPU-6050 I2C driver
│   │   ├── ble_service/                NimBLE GATT server
│   │   ├── exercise_tracker/           Rep counting state machine
│   │   ├── battery/                    ADC and LUT battery estimation
│   │   └── haptic/                     Vibration motor driver
│   ├── tools/ble_plot.py               Live BLE plotting (Python debug tool)
│   ├── docs/                           Proposals, math reference, test notes
│   └── platformio.ini                  Build configuration
├── android/
│   ├── app/src/main/java/.../
│   │   ├── data/ble/                   BleManager, RehabPacketParser, BleConstants
│   │   ├── domain/model/               RehabPacket, ExerciseType, CalibrationStatus
│   │   ├── ui/
│   │   │   ├── live/                   LiveSessionScreen, LiveSessionViewModel
│   │   │   ├── device/                 DeviceScreen, DeviceViewModel, BlePermissions
│   │   │   ├── history/                HistoryScreen
│   │   │   ├── settings/               SettingsScreen
│   │   │   ├── navigation/             NavGraph (bottom nav and routes)
│   │   │   └── theme/                  Color, Type, Theme (Material 3)
│   │   ├── AclRehabApp.kt             Hilt application entry point
│   │   └── MainActivity.kt            Single-activity Compose host
│   ├── build.gradle.kts               AGP 9.1.0, Compose, Hilt, Room, KSP
│   └── gradle/libs.versions.toml      Central version catalog
└── README.md
```

## Installation

### Firmware Setup

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI).

```bash
cd firmware
pio run -t upload
```

The `platformio.ini` file configures the ESP32-S3 target, library dependencies, and build flags.

### Android Setup

Requires [Android Studio](https://developer.android.com/studio) (Ladybug or newer for AGP 9.x support).

1. Open the `android/` directory in Android Studio.
2. Let Gradle sync complete (uses AGP 9.1.0 with bundled Kotlin 2.2.10).
3. Connect an Android device (minSdk 26 / Android 8.0+) with USB debugging enabled.
4. Run the app via **Run > Run 'app'** or the green play button.

**Note:** AGP 9.x bundles Kotlin, **do not** add a separate `kotlin-android` plugin.

## Usage

### Device Pairing

1. Power on the sensor strap (ESP32 boots and begins BLE advertising as `ACL-Rehab`).
2. Open the app and navigate to the **Device** tab.
3. Tap **Scan** and the device should appear within a few seconds.
4. Tap the device name to connect.
5. Once connected, switch to the **Live** tab to see real-time data.

The app will automatically attempt to reconnect up to 5 times if the connection drops.

### Exercises

1. Strap the device with the thigh sensor above the knee and the shank sensor below.
2. Hold still for 3 seconds while the gyroscope calibrates (status shown on Live tab).
3. Press the button on the device to cycle through exercises.
4. Perform reps — the device counts them and provides haptic feedback on completion.

## Documentation

Detailed documentation is available in [`firmware/docs/`](firmware/docs/) (more will be added on completion):

| Document                                     | Description                 |
|----------------------------------------------|-----------------------------|
| [schematic.pdf](firmware/docs/schematic.pdf) | Prototype Circuit schematic |
