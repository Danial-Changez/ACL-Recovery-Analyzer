#ifndef EXERCISE_H
#define EXERCISE_H

#include <cstdint>
#include "types.h"

/** @brief Rep-counting state machine. */
enum class ExState : uint8_t {
    IDLE,
    EXTENDING,
    PEAK_REACHED,
    RETURNING
};

/**
 * @brief Exercise session tracking and rep counting.
 *
 * State machine that detects reps across 5 ACL rehab exercises.
 * Selects the correct angle source and direction based on the
 * active exercise profile.
 */
class ExerciseTracker {
  public:
    static void                    init();
    static void                    update(float kneeAngle, float thighRoll, float thighPitch, uint32_t nowMs);
    static ExerciseMetrics         getMetrics();
    static void                    resetSession();
    static void                    setExercise(ExerciseType type);
    static void                    nextExercise();
    static const ExerciseProfile&  getProfile();
    static ExerciseType            getExerciseType();
};

#endif /* EXERCISE_H */
