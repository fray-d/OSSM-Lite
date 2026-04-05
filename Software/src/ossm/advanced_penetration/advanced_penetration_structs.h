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
static const char ma[] PROGMEM = "MA";
static const char m1[] PROGMEM = "M1";
static const char m2[] PROGMEM = "M2";
static const char m3[] PROGMEM = "M3";
static const char m4[] PROGMEM = "M4";
static const char mo[] PROGMEM = "MO";
static const char sp[] PROGMEM = "SP";

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
    String name;
    float getNormalized(){
        return value / 100.0;
    }
    float getRampValue(float exp = 0.8){
        return pow(1 - pow(1 - getNormalized(), exp), 1 / exp);
    };
};

struct ModifierControl : public Control {
    ModifierControls id;
    String encodeString(bool details = false) {
        String output;
        if (details) {
            output += name + "(";
            output += String(minValue) + "/";
            output += String(maxValue) + ")";
        } else {
            output += String(value);
        }
        return output;
    }
    bool processStringCommand(ModifierControls control, String cmd){
        if (control == id){
            u8_t v = cmd.toInt();
            if (v >= minValue && v <= maxValue) {
                value = v;
                return true;
            }
        }
        return false;
    }
};

struct Modifier {
    ModifierControl amplitude = {100, 0, 100, ma, ModifierControls::AMPLITUDE};
    ModifierControl inStep = {1, 1, 25, m1, ModifierControls::IN_STEP};
    ModifierControl inWait = {0, 0, 25, m2, ModifierControls::IN_WAIT};
    ModifierControl outStep = {1, 1, 25, m3, ModifierControls::OUT_STEP};
    ModifierControl outWait = {0, 0, 25, m4, ModifierControls::OUT_WAIT};
    ModifierControl offset = {0, 0, 100, mo, ModifierControls::OFFSET};
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
    String encodeString(bool details = false) {
        String output = ":" + amplitude.encodeString(details);
        output += ":" + inStep.encodeString(details);
        output += ":" + inWait.encodeString(details);
        output += ":" + outStep.encodeString(details);
        output += ":" + outWait.encodeString(details);
        output += ":" + offset.encodeString(details);
        return output;
    }
};

struct BaseControl : Control {
    BaseControls id;
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
    String encodeString(bool details = false) {
        String output;
        if (details) {
            output += name + "(";
            output += String(minValue) + "/";
            output += String(maxValue) + ")";
            if (id == BaseControls::DEPTH_1) {
                if (modifier == nullptr) {
                    modifier = new Modifier;
                }
                output += modifier->encodeString(true);
            }
        } else {
            output += String(value);
            if (modifier != nullptr && modifier->active()) {
                output += modifier->encodeString();
            }
        }
        return output;
    }
    bool processStringCommand(BaseControls control, String cmd){
        if (control == id){
            u8_t i = cmd.indexOf(":");
            if (i == 255){
                u8_t v = cmd.toInt();
                if (v >= minValue && v <= maxValue) {
                    value = v;
                    return true;
                }
                return false;
            }
            if (modifier == nullptr) {
                modifier = new Modifier;
            }
            ModifierControls control = static_cast<ModifierControls>(cmd.substring(0,i).toInt());
            cmd = cmd.substring(i+1);
            return modifier->amplitude.processStringCommand(control, cmd)
                    || modifier->inStep.processStringCommand(control, cmd)
                    || modifier->inWait.processStringCommand(control, cmd)
                    || modifier->outStep.processStringCommand(control, cmd)
                    || modifier->outWait.processStringCommand(control, cmd)
                    || modifier->offset.processStringCommand(control, cmd);
            
        }
        return false;
    }
};

struct Settings {
    BaseControl speed = {0, 0, 100, sp, BaseControls::BASE_COUNT};
    BaseControl inSpeed = {100, 1, 100, s1, BaseControls::SPEED_1};
    BaseControl outSpeed = {100, 1, 100, s2, BaseControls::SPEED_2};
    BaseControl maxDepth = {10, 0, 100, d1, BaseControls::DEPTH_1};
    BaseControl minDepth = {0, 0, 100, d2, BaseControls::DEPTH_2};
    BaseControl inAcceleration = {40, 0, 100, a1, BaseControls::ACCEL_1};
    BaseControl outAcceleration = {40, 0, 100, a2, BaseControls::ACCEL_2};
    ControlStatus status = ControlStatus::BASE_MENU;
    ControlStatus lastStatus = ControlStatus::STATUS_COUNT;
    BaseControls baseControl = BaseControls::DEPTH_1;
    ModifierControls modifierControl = ModifierControls::AMPLITUDE;
    bool changed = false;
    String commandString;
    u8_t usableDepth() {
        return maxDepth.value - minDepth.value;
    };
    String getStatus(bool details = false) {
        String output = maxDepth.encodeString(details) + ",";
        output += minDepth.encodeString(details) + ",";
        output += inSpeed.encodeString(details) + ",";
        output += outSpeed.encodeString(details) + ",";
        output += inAcceleration.encodeString(details) + ",";
        output += outAcceleration.encodeString(details) + ",";
        output += speed.encodeString(details);
        ESP_LOGD("Advanced", "Status: %s", output.c_str());
        return output;
    }
    bool processStringCommand(String cmd) {
        cmd = commandString + cmd;
        int i = cmd.lastIndexOf(',');
        if (i < 0){
            commandString = "";
            return false;
        }
        commandString = cmd.substring(i+1);
        cmd = cmd.substring(0,i+1);

        i = cmd.indexOf(',');
        BaseControls control;
        while (i > 0){
            String single = cmd.substring(0,i);
            cmd = cmd.substring(i+1);
            i = cmd.indexOf(',');

            int j = single.indexOf(':');
            if (j < 0) {
                return false;
            }
            control = static_cast<BaseControls>(single.substring(0,j).toInt());
            single = single.substring(j+1);
            changed = maxDepth.processStringCommand(control, single)
                    || minDepth.processStringCommand(control, single)
                    || inSpeed.processStringCommand(control, single)
                    || outSpeed.processStringCommand(control, single)
                    || inAcceleration.processStringCommand(control, single)
                    || outAcceleration.processStringCommand(control, single)
                    || speed.processStringCommand(control, single);

        }
        lastStatus = ControlStatus::STATUS_COUNT;
        return changed;
    }
} currentSettings;

struct StrokeMath {
    float accelTime;
    float accel;
    float totalTime;
};

}

#endif