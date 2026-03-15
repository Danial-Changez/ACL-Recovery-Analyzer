#include "exercise_tracker.h"
#include "config.h"
#include "filters.h"
#include <cmath>

/** Exercise profile table
 * 
 * SEATED_KNEE_EXT:  starts at ~90deg, extends to 50deg
 * HIP_ABDUCTION:    thigh roll from 0deg out to 30deg
 * SINGLE_LEG_SQUAT: knee angle from ~0deg to 40deg
 * LEG_BRIDGING:     knee angle from ~45deg to 90deg
 * REVERSE_LUNGE:    knee angle from ~0deg to 90deg
 */
static const ExerciseProfile profiles[] = {
    {ExerciseType::SEATED_KNEE_EXT,  AngleSource::KNEE_ANGLE,  85.0f, 50.0f, true,  25.0f, "Seated Knee Ext"},
    {ExerciseType::HIP_ABDUCTION,    AngleSource::THIGH_ROLL,   5.0f, 25.0f, false,  0.0f, "Hip Abduction"},
    {ExerciseType::SINGLE_LEG_SQUAT, AngleSource::KNEE_ANGLE,  10.0f, 35.0f, false, 20.0f, "Single-Leg Squat"},
    {ExerciseType::LEG_BRIDGING,     AngleSource::KNEE_ANGLE,  40.0f, 80.0f, false, 22.5f, "Leg Bridging"},
    {ExerciseType::REVERSE_LUNGE,    AngleSource::KNEE_ANGLE,  10.0f, 80.0f, false, 45.0f, "Reverse Lunge"},
};

#define NUM_PROFILES  (sizeof(profiles) / sizeof(profiles[0]))

/** Internal state */
static ExState                state          = ExState::IDLE;
static ExerciseType           currentType    = ExerciseType::SEATED_KNEE_EXT;
static const ExerciseProfile* pActiveProfile = &profiles[0];

static uint8_t repCount            = 0;
static float   currentAngle        = 0.0f;
static float   extremeAngleThisRep = 0.0f;
static float   maxRomSession       = 0.0f;
static float   romSum              = 0.0f;
static bool    repJustCompleted    = false;

static LowPassFilter angleFilter;
static float selectAngle(float kneeAngle, float thighRoll, float thighPitch) {
    switch (pActiveProfile->m_source) {
        case AngleSource::KNEE_ANGLE:  return kneeAngle;
        case AngleSource::THIGH_ROLL:  return fabsf(thighRoll);
        case AngleSource::THIGH_PITCH: return thighPitch;
        default:                       return kneeAngle;
    }
}

static bool pastStart(float angle) {
    return angle > pActiveProfile->m_startAngle;
}

static bool pastPeak(float angle) {
    if (pActiveProfile->m_decreasing) {
        return angle < pActiveProfile->m_peakAngle;
    }
    return angle > pActiveProfile->m_peakAngle;
}

static bool belowStart(float angle) {
    return angle < pActiveProfile->m_startAngle;
}

static bool belowPeak(float angle) {
    if (pActiveProfile->m_decreasing) {
        return angle > pActiveProfile->m_peakAngle;
    }
    return angle < pActiveProfile->m_peakAngle;
}

static bool isMoreExtreme(float a, float b) {
    if (pActiveProfile->m_decreasing) {
        return a < b;
    }
    return a > b;
}

static float romForRep(float extreme) {
    return fabsf(extreme - pActiveProfile->m_startAngle);
}

/** @brief Initialize filter and reset session state. */
void ExerciseTracker::init() {
    angleFilter.configure(ROM_LPF_ALPHA);
    state               = ExState::IDLE;
    repCount            = 0;
    currentAngle        = 0.0f;
    extremeAngleThisRep = 0.0f;
    maxRomSession       = 0.0f;
    romSum              = 0.0f;
    repJustCompleted    = false;
}

/**
 * @brief Update with all available angles.
 *
 * The tracker picks the correct angle based on the active exercise profile.
 * 
 * @param [in] kneeAngle Relative knee flexion angle (degrees).
 * @param [in] thighRoll Thigh frontal-plane angle (degrees).
 * @param [in] thighPitch Thigh sagittal-plane angle (degrees).
 * @param [in] nowMs Current timestamp (milliseconds).
 */
void ExerciseTracker::update(float kneeAngle, float thighRoll, float thighPitch, uint32_t nowMs) {
    (void)nowMs;

    float raw    = selectAngle(kneeAngle, thighRoll, thighPitch);
    currentAngle = angleFilter.apply(raw);
    repJustCompleted = false;

    switch (state) {
        case ExState::IDLE:
            if (pastStart(currentAngle)) {
                state               = ExState::EXTENDING;
                extremeAngleThisRep = currentAngle;
            }
            break;

        case ExState::EXTENDING:
            if (isMoreExtreme(currentAngle, extremeAngleThisRep)) {
                extremeAngleThisRep = currentAngle;
            }

            if (pastPeak(currentAngle)) {
                state = ExState::PEAK_REACHED;
            } else if (belowStart(currentAngle)) {
                state = ExState::IDLE;
            }
            break;

        case ExState::PEAK_REACHED:
            if (isMoreExtreme(currentAngle, extremeAngleThisRep)) {
                extremeAngleThisRep = currentAngle;
            }

            if (belowPeak(currentAngle)) {
                state = ExState::RETURNING;
            }
            break;

        case ExState::RETURNING:
            if (belowStart(currentAngle)) {
                repCount++;
                float rom = romForRep(extremeAngleThisRep);
                romSum += rom;
                
                if (rom > maxRomSession) {
                    maxRomSession = rom;
                }
                
                repJustCompleted = true;
                state            = ExState::IDLE;
            } else if (pastPeak(currentAngle)) {
                state = ExState::PEAK_REACHED;
            }
            break;
    }
}

/** @brief Get current session metrics. */
ExerciseMetrics ExerciseTracker::getMetrics() {
    ExerciseMetrics m;
    m.m_repCount          = repCount;
    m.m_currentAngle      = currentAngle;
    m.m_maxRomThisSession = maxRomSession;
    m.m_avgRomPerRep      = (repCount > 0) ? (romSum / (float)repCount) : 0.0f;
    m.m_exerciseActive    = (state != ExState::IDLE);
    m.m_repJustCompleted  = repJustCompleted;
    return m;
}

/** @brief Reset session counters and state. */
void ExerciseTracker::resetSession() {
    init();
}

/** @brief Set exercise type -- resets session and applies profile. */
void ExerciseTracker::setExercise(ExerciseType type) {
    uint8_t idx = static_cast<uint8_t>(type);
    
    if (idx >= NUM_PROFILES) {
        idx = 0;
    }
    
    currentType    = static_cast<ExerciseType>(idx);
    pActiveProfile = &profiles[idx];
    init();
}

/** @brief Cycle to next exercise type, wrapping around. */
void ExerciseTracker::nextExercise() {
    uint8_t next = (static_cast<uint8_t>(currentType) + 1) % NUM_PROFILES;
    setExercise(static_cast<ExerciseType>(next));
}

/** @brief Get the current exercise profile. */
const ExerciseProfile& ExerciseTracker::getProfile() {
    return *pActiveProfile;
}

/** @brief Get current exercise type. */
ExerciseType ExerciseTracker::getExerciseType() {
    return currentType;
}
