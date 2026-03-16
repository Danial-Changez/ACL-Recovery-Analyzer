package com.example.aclrecoveryanalyzer.domain.model

enum class ExerciseType(val value: Int) {
    NONE(0),
    QUAD_SET(1),
    STRAIGHT_LEG_RAISE(2),
    HEEL_SLIDE(3),
    WALKING(4);

    companion object {
        fun fromValue(value: Int): ExerciseType =
            entries.firstOrNull { it.value == value } ?: NONE
    }
}
