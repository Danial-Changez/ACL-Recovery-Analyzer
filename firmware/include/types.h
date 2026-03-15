#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

/**
 * @brief Euler angles from the Madgwick filter.
 *
 * Orientation in degrees. Pitch is the primary
 * axis for knee flexion measurement.
 */
struct EulerAngles {
    float m_pitch;
    float m_roll;
    float m_heading;
};

/**
 * @brief 3D vector for accel, gyro, and magnetometer.
 */
struct Vec3 {
    float m_x;
    float m_y;
    float m_z;
};

/**
 * @brief Per-sensor calibration flags set during startup.
 *
 * Members:
 *
 * m_gyroStable — Gyroscope variance below threshold.
 *
 * m_accelStable — Accelerometer variance below threshold.
 */
struct CalibStatus {
    bool m_gyroStable  : 1;
    bool m_accelStable : 1;
};

/**
 * @brief Exercise types corresponding to ACL rehab stages.
 *
 * Each type defines which angle to track and the rep thresholds.
 * Seated knee extension tracks decreasing angle (90->50),
 * all others track increasing angle from ~0.
 */
enum class ExerciseType : uint8_t {
    SEATED_KNEE_EXT  = 0, /* Stage 1: knee angle 90->50 */
    HIP_ABDUCTION    = 1, /* Stage 1: thigh roll 0->30  */
    SINGLE_LEG_SQUAT = 2, /* Stage 2: knee angle 0->40  */
    LEG_BRIDGING     = 3, /* Stage 3: knee angle 45->90  */
    REVERSE_LUNGE    = 4, /* Stage 4: knee angle 0->90  */
    NUM_EXERCISES    = 5
};

/**
 * @brief Angle source for an exercise profile.
 */
enum class AngleSource : uint8_t {
    KNEE_ANGLE,  /* shank.pitch - thigh.pitch */
    THIGH_ROLL,  /* thigh roll (frontal plane) */
    THIGH_PITCH  /* thigh pitch (sagittal) */
};

/**
 * @brief Configuration for a single exercise type.
 *
 * Members:
 *
 * m_type — ExerciseType enum value.
 *
 * m_source — Which angle to track (knee, thigh roll/pitch).
 *
 * m_startAngle — Angle where rep begins (degrees).
 *
 * m_peakAngle — Angle that must be reached (degrees).
 *
 * m_decreasing — True if rep goes from high to low angle.
 *
 * m_maxSpeed — Max angular velocity (deg/s), 0 = no limit.
 *
 * m_pName — Human-readable exercise name.
 */
struct ExerciseProfile {
    ExerciseType m_type;
    AngleSource  m_source;
    float        m_startAngle; /* angle where rep begins */
    float        m_peakAngle;  /* angle that must be reached */
    bool         m_decreasing; /* true if rep goes from high->low angle */
    float        m_maxSpeed;   /* max angular velocity (deg/s), 0=no limit */
    const char*  m_pName;
};

/**
 * @brief State and statistics for a single exercise session.
 *
 * m_repJustCompleted is true for exactly one cycle after a rep
 * finishes, allowing one-shot feedback events.
 *
 * Members:
 *
 * m_repCount — Total reps completed this session.
 *
 * m_currentAngle — Live angle being tracked (degrees).
 *
 * m_maxRomThisSession — Peak ROM achieved this session.
 *
 * m_avgRomPerRep — Mean peak ROM across all reps.
 *
 * m_exerciseActive — True while a rep is in progress.
 *
 * m_repJustCompleted — True for one cycle after rep finishes.
 */
struct ExerciseMetrics {
    uint8_t m_repCount;
    float   m_currentAngle;      /* degrees */
    float   m_maxRomThisSession; /* peak flexion achieved */
    float   m_avgRomPerRep;      /* mean peak flexion across reps */
    bool    m_exerciseActive   : 1;
    bool    m_repJustCompleted : 1;
};

/**
 * @brief Packed for BLE transmission — 23 bytes total, little-endian.
 *
 * Angle and acceleration fields are scaled x100 for 0.01
 * resolution without floating point.
 *
 * Members:
 *
 * kneeAngle — Relative knee angle, x100.
 *
 * thighPitch — Thigh pitch, x100.
 *
 * thighRoll — Thigh roll, x100.
 *
 * thighHeading — Thigh heading, x100.
 *
 * shankPitch — Shank pitch, x100.
 *
 * shankRoll — Shank roll, x100.
 *
 * shankHeading — Shank heading, x100.
 *
 * linearAccelZ — Shank Z accel, x100.
 *
 * stepCount — Reserved (unused).
 *
 * repCount — Reps completed.
 *
 * exerciseType — ExerciseType enum value.
 *
 * calibStatus — bit 0: gyroStable, bit 1: accelStable.
 *
 * batteryPercent — Charge level (0–100).
 *
 * flags — bit 0: exerciseActive, bit 2: lowBattery, bit 3: calibIncomplete.
 */
struct __attribute__((packed)) RehabPacket {
    int16_t  kneeAngle;    /* shank.pitch - thigh.pitch, x100 */
    int16_t  thighPitch;   /* x100 */
    int16_t  thighRoll;    /* x100 */
    int16_t  thighHeading; /* x100 */
    int16_t  shankPitch;   /* x100 */
    int16_t  shankRoll;    /* x100 */
    int16_t  shankHeading; /* x100 */
    int16_t  linearAccelZ; /* x100 */
    uint16_t stepCount;
    uint8_t  repCount;
    uint8_t  exerciseType; /* ExerciseType enum value */
    uint8_t  calibStatus;
    uint8_t  batteryPercent;
    uint8_t  flags;
};

#endif /* TYPES_H */
