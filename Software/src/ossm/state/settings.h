#ifndef OSSM_STATE_SETTINGS_H
#define OSSM_STATE_SETTINGS_H

#include "ui.h"
#include "../lib/StrokeEngine/src/StrokeEngine.h"

extern struct SettingPercents {
    float speed;
    float sensation;
    float maxPosition;
    float minPosition;
    float stroke;
    float buffer;
    float speedKnob;
    float speedBLE;
    StrokePatterns pattern;
    ui::PlayControls playControl = ui::PlayControls::MIN_POSITION;
} settings;

#endif  // OSSM_STATE_SETTINGS_H
