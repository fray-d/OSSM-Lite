#ifndef OSSM_ADVANCED_PENETRATION_STRUCTS_H
#define OSSM_ADVANCED_PENETRATION_STRUCTS_H
#include "Progmem.h"

namespace advanced_penetration {

static const char d1[] PROGMEM = "+D";
static const char d2[] PROGMEM = "-D";
static const char s1[] PROGMEM = "+S";
static const char s2[] PROGMEM = "-S";
static const char a1[] PROGMEM = "+A";
static const char a2[] PROGMEM = "-A";

enum ControlStatus {
    BASE_MENU,
    BASE_VALUE,
    MODIFIER_MENU,
    MODIFIER_VALUE,
    STATUS_COUNT
};

enum BaseControls {
    DEPTH_1,
    DEPTH_2,
    SPEED_1,
    SPEED_2,
    ACCEL_1,
    ACCEL_2,
    BASE_COUNT
};

enum ModifierControls {
    AMPLITUDE,
    IN_STEP,
    IN_WAIT,
    OUT_STEP,
    OUT_WAIT,
    OFFSET,
    GO_BACK,
    MODIFIER_COUNT
};

struct Control {
    u8_t value;
    u8_t minValue = 0;
    u8_t maxValue = 100;
    float getNormalized(){
        return value / 100.0;
    }
    float getRampValue(float exp = 0.8){
        return pow(1 - pow(1 - getNormalized(), exp), 1 / exp);
    };
};

struct ModifierControl : public Control {
    ModifierControls id;
};

struct Modifier {
    ModifierControl amplitude = {100, 0, 100, ModifierControls::AMPLITUDE};
    ModifierControl inStep = {1, 1, 25, ModifierControls::IN_STEP};
    ModifierControl inWait = {0, 0, 25, ModifierControls::IN_WAIT};
    ModifierControl outStep = {1, 1, 25, ModifierControls::OUT_STEP};
    ModifierControl outWait = {0, 0, 25, ModifierControls::OUT_WAIT};
    ModifierControl offset = {0, 0, 100, ModifierControls::OFFSET};
    u8_t stepCount() {
        return inStep.value + inWait.value + outStep.value + outWait.value;
    }
    ModifierControls getControlForStep(int cycle) {
        cycle = (cycle + offset.value) % stepCount();
        if (cycle < inStep.value){
            return ModifierControls::IN_STEP;
        } else {
            cycle -= inStep.value;
        }
        if (cycle < inWait.value) {
            return ModifierControls::IN_WAIT;
        } else {
            cycle -= inWait.value;
        }
        if (cycle < outStep.value) {
            return ModifierControls::OUT_STEP;
        } 
        return ModifierControls::OUT_WAIT;
    }
    float getModification(int cycle){
        float ratio = (100 - amplitude.value) / 100.0;
        if (cycle < 0) {
            return 1 - ratio;
        }
        cycle = (cycle + offset.value) % stepCount();
        if (cycle < inStep.value){
            float slice = ratio / inStep.value * (cycle + 1);
            return 1 - slice;
        } else {
            cycle -= inStep.value;
        }
        if (cycle < inWait.value) {
            return 1 - ratio;
        } else {
            cycle -= inWait.value;
        }
        if (cycle < outStep.value) {
            float slice = ratio / outStep.value * (cycle + 1);
            return 1 - ratio + slice;
        } 
        return 1;
    }
    bool active() {
        return amplitude.value < 100;
    }
};

struct BaseControl : Control {
    BaseControls id;
    String name;
    Modifier* modifier;
    u8_t getModifiedValue(int strokeCount = -1){
        if (modifier == nullptr) {
            return value;
        }
        int8_t cycle = (strokeCount / 2) % modifier->stepCount();
        int8_t difference = value - minValue;
        if (id == BaseControls::DEPTH_2){
            difference = value - maxValue;
        }
        if (strokeCount < 0) {
            cycle = -1;
        }
        return value - difference * (1 - modifier->getModification(cycle));
    }
    float getNormalizedModifiedValue(int strokeCount = -1) {
        return getModifiedValue(strokeCount) / 100.0;
    }
    float getRampedModifiedValue(float exp = 0.8, int strokeCount = -1) {
        return pow(1 - pow(1 - getNormalizedModifiedValue(strokeCount),exp), 1 / exp);
    }
};

struct Settings {
    BaseControl speed = {0, 0, 100, BaseControls::BASE_COUNT};
    BaseControl inSpeed = {100, 1, 100, BaseControls::SPEED_1, s1};
    BaseControl outSpeed = {100, 1, 100, BaseControls::SPEED_2, s2};
    BaseControl maxDepth = {10, 0, 100, BaseControls::DEPTH_1, d1};
    BaseControl minDepth = {0, 0, 100, BaseControls::DEPTH_2, d2};
    BaseControl inAcceleration = {40, 0, 100, BaseControls::ACCEL_1, a1};
    BaseControl outAcceleration = {40, 0, 100, BaseControls::ACCEL_2, a2};
    ControlStatus status = ControlStatus::BASE_MENU;
    ControlStatus lastStatus = ControlStatus::STATUS_COUNT;
    BaseControls baseControl = BaseControls::DEPTH_1;
    ModifierControls modifierControl = ModifierControls::AMPLITUDE;
    bool changed = false;
    u8_t usableDepth() {
        return maxDepth.value - minDepth.value;
    };
} currentSettings;

struct StrokeMath {
    float accelTime;
    float accel;
    float totalTime;
};

}

#endif