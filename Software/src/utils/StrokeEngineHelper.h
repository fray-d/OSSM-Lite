#ifndef STROKE_ENGINE_HELPER_H
#define STROKE_ENGINE_HELPER_H
#include <config.h>

#include "../lib/StrokeEngine/src/StrokeEngine.h"
#include "constants/Config.h"
#include "constants/Pins.h"
#include "services/UserConfig.h"

/*#################################################################################################
##
##    G L O B A L    D E F I N I T I O N S   &   D E C L A R A T I O N S
##
##################################################################################################*/

// enum of stroke engine states
enum PlayControls { MAX_POSITION = 0, MIN_POSITION = 1, SENSATION = 2, BUFFER = 3 };

static machineProperties properties{
    .maxSpeed = Config::Driver::maxSpeedMmPerSecond,
    .maxAcceleration = Config::Driver::maxAcceleration,
    .stepsPerMillimeter = Config::Driver::motorStepPerRevolution /
                    (Config::Driver::pulleyToothCount * Config::Driver::beltPitchMm)
};

static bool isChangeSignificant(float oldPct, float newPct) {
    return oldPct != newPct &&
           (abs(newPct - oldPct) > 2 || newPct == 0 || newPct == 100);
}

static float calculateSensation(float sensationPercentage) {
    return float((sensationPercentage * 200.0) / 100.0) - 100.0f;
}

#endif
