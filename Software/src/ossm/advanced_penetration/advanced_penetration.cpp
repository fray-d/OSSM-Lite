#include "advanced_penetration.h"

#include "constants/Config.h"
#include "utils/analog.h"
#include "ui.h"
#include "services/display.h"
#include "ossm/state/calibration.h"
#include "ossm/state/state.h"
#include "services/encoder.h"
#include "services/stepper.h"
#include "services/tasks.h"
#include "Strings.h"

namespace sml = boost::sml;
using namespace sml;

namespace advanced_penetration {

static const char d1[] PROGMEM = "D+";
static const char d2[] PROGMEM = "D-";
static const char s1[] PROGMEM = "S+";
static const char s2[] PROGMEM = "S-";
static const char a1[] PROGMEM = "A+";
static const char a2[] PROGMEM = "A-";

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
    ModifierControl amplitude = {0, 0, 100, ModifierControls::AMPLITUDE};
    ModifierControl inStep = {1, 1, 25, ModifierControls::IN_STEP};
    ModifierControl inWait = {0, 0, 25, ModifierControls::IN_WAIT};
    ModifierControl outStep = {1, 1, 25, ModifierControls::OUT_STEP};
    ModifierControl outWait = {0, 0, 25, ModifierControls::OUT_WAIT};
    ModifierControl offset = {0, 0, 100, ModifierControls::OFFSET};
    u8_t stepCount() {
        return inStep.value + inWait.value + outStep.value + outWait.value;
    }
    float getModification(int cycle){
        float ratio = amplitude.value / 100.0;
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
        return amplitude.value > 0;
    }
};

struct BaseControl : Control {
    BaseControls id;
    String name;
    Modifier* modifier;
    u8_t getModifiedValue(int strokeCount){
        if (modifier == nullptr) {
            return value;
        }
        int cycle = (strokeCount / 2) % modifier->stepCount();
        int8_t difference = value - minValue;
        if (id == BaseControls::DEPTH_2){
            difference = value - maxValue;
        }
        return value - difference * (1 - modifier->getModification(cycle));
    }
    float getNormalizedModifiedValue(int strokeCount) {
        return getModifiedValue(strokeCount) / 100.0;
    }
    float getRampedModifiedValue(int strokeCount, float exp = 0.8) {
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

u8_t textPosition = 19;

void centeredText(String controlText) {
    uint16_t stringWidth = display.getUTF8Width(controlText.c_str());
    display.drawUTF8(120 - stringWidth/2, textPosition, controlText.c_str());
    textPosition += 9;
}

void setEncoderIfStatus(Control& c, ControlStatus status){
    if (currentSettings.status == status) {
        display.setFont(u8g2_font_timB08_tn);
        if (currentSettings.lastStatus != currentSettings.status) {
            encoder.setBoundaries(c.minValue, c.maxValue, false);
            encoder.setEncoderValue(c.value - 1);
        }
        c.value = constrain(encoder.readEncoder(), c.minValue, c.maxValue);
    }
    currentSettings.lastStatus = currentSettings.status;
}

void drawModifierControl(ModifierControl& m) {
    display.setFont(u8g2_font_timR08_tn);
    if (currentSettings.modifierControl == m.id) {
        setEncoderIfStatus(m, ControlStatus::MODIFIER_VALUE);
    }
    centeredText(String(m.value));
}

void selectText(u8_t index) {
    display.drawFrame(111,10 + 9 * index,17,11);
}

void drawModifier(BaseControl& a) {
    if (a.modifier == nullptr) {
        a.modifier = new Modifier;
    }
    drawModifierControl(a.modifier->amplitude);
    drawModifierControl(a.modifier->inStep);
    drawModifierControl(a.modifier->inWait);
    drawModifierControl(a.modifier->outStep);
    drawModifierControl(a.modifier->outWait);
    drawModifierControl(a.modifier->offset);
    selectText(currentSettings.modifierControl);

    u8_t steps = a.modifier->stepCount();
    u8_t barWdith = 101 / steps;
    u8_t w = barWdith * steps;
    u8_t x = 112 - w;
    for (int step = 0; step < steps; step ++) {
        u8_t value = 54 * a.modifier->getModification(step);
        display.drawBox(x, display.getHeight() - value, barWdith, value);
        x += barWdith;
    }
}

struct StrokeMath {
    float accelTime;
    float accel;
    float totalTime;
};

StrokeMath calculate(u8_t speed, float accelValue) {
    StrokeMath stroke;
    float minAccel = speed / (currentSettings.usableDepth() / float(speed));
    stroke.accel = minAccel + (minAccel * 9) * accelValue;
    stroke.accelTime = speed / stroke.accel;
    float accelDistance = 0.5 * stroke.accel * pow(stroke.accelTime,2);
    float maxSpeedDistance = currentSettings.usableDepth() - accelDistance * 2;
    float maxSpeedTime = maxSpeedDistance / speed;
    stroke.totalTime = stroke.accelTime * 2 + maxSpeedTime;
    return stroke;
}

void drawStroke(u8_t x=10, u8_t y=10, u8_t w=101, u8_t h=54) {
    StrokeMath inStroke = calculate(currentSettings.inSpeed.value, currentSettings.inAcceleration.getRampValue(0.6));
    StrokeMath outStroke = calculate(currentSettings.outSpeed.value, currentSettings.outAcceleration.getRampValue(0.6));
    u8_t split = w * (inStroke.totalTime/ (inStroke.totalTime+outStroke.totalTime));
    float inSlice = inStroke.totalTime / split;
    float outSlice = outStroke.totalTime / (w-split);
    u8_t x1 = x - 1;
    u8_t y1 = 63 - (currentSettings.minDepth.getNormalized() * h);
    u8_t x2 = x + split;
    u8_t y2 = 63 - (currentSettings.maxDepth.getNormalized() * h);
    u8_t x3 = x + w;
    u8_t lx1 = 0;
    u8_t ly1 = 0;
    u8_t lx2 = 0;
    u8_t ly2 = 0;

    for (int px = 0; px < split; px++){
        if (px <= inStroke.accelTime / inSlice) {
            u8_t py = 0.5 * inStroke.accel * pow(inSlice * px,2);
            py = py / 100.0 * h;
            lx1 = x1 + px;
            lx2 = x2 - px;
            ly1 = y1 - py;
            ly2 = y2 + py;
            display.drawPixel(lx1,ly1);
            display.drawPixel(lx2,ly2);
        }
    }
    display.drawLine(lx1,ly1,lx2,ly2);

    for (int px = 0; px < (w-split); px++){
        if (px <= outStroke.accelTime / outSlice) {
            u8_t py = 0.5 * outStroke.accel * pow(outSlice * px,2);
            py = py / 100.0 * h;
            lx1 = x2 + px;
            lx2 = x3 - px;
            ly1 = y2 + py;
            ly2 = y1 - py;
            display.drawPixel(lx1,ly1);
            display.drawPixel(lx2,ly2);
        }
    }
    display.drawLine(lx1,ly1,lx2,ly2);
}

void updateControl(BaseControl& a, u8_t minVal = 0, u8_t maxVal = 100) {
    display.setFont(u8g2_font_timR08_tf);
    if (currentSettings.status == ControlStatus::MODIFIER_MENU
                || currentSettings.status == ControlStatus::MODIFIER_VALUE){
        if (currentSettings.baseControl == a.id) {
            drawModifier(a);
        }
        return;
    }
    a.minValue = minVal;
    a.maxValue = maxVal;
    if (currentSettings.baseControl == a.id) {
        selectText(currentSettings.baseControl);
        setEncoderIfStatus(a, ControlStatus::BASE_VALUE);
        drawStroke();
    }
    if (encoder.readEncoder()/3 >= BaseControls::BASE_COUNT && currentSettings.status == ControlStatus::BASE_MENU) {
        if (a.modifier != nullptr && a.modifier->active()) {
            display.setFont(u8g2_font_timB08_tf);
        }
        centeredText(a.name);
        return;
    }
    centeredText(String(a.value));
}

void advancedClick() {
    u8_t c = 100;
    bool loop = true;
    u8_t value = 0;
    switch (currentSettings.status) {
        case ControlStatus::BASE_MENU:
            if (encoder.readEncoder()/3 >= BaseControls::BASE_COUNT) {
                currentSettings.status = ControlStatus::MODIFIER_MENU;
                c = ModifierControls::MODIFIER_COUNT * 3;
            } else {
                currentSettings.status = ControlStatus::BASE_VALUE;
                loop = false;
            }
            break;
        case ControlStatus::BASE_VALUE:
            currentSettings.status = ControlStatus::BASE_MENU;
            value = currentSettings.baseControl * 3;
            c = BaseControls::BASE_COUNT * 6;
            break;
        case ControlStatus::MODIFIER_MENU:
            if (currentSettings.modifierControl == ModifierControls::GO_BACK) {
                currentSettings.status = ControlStatus::BASE_MENU;
                value = (currentSettings.baseControl + BaseControls::BASE_COUNT) * 3;
                c = BaseControls::BASE_COUNT * 6;
            } else {
                currentSettings.status = ControlStatus::MODIFIER_VALUE;
                loop = false;
            }
            break;
        case ControlStatus::MODIFIER_VALUE:
            currentSettings.status = ControlStatus::MODIFIER_MENU;
            value = currentSettings.modifierControl * 3;
            c = ModifierControls::MODIFIER_COUNT * 3;
            break;
        default:
            break;
    }
    encoder.setBoundaries(0, c - 1, loop);
    encoder.setEncoderValue(value);
}

bool isInCorrectState() {
    return stateMachine->is("advancedPenetration"_s)
        || stateMachine->is("advancedPenetration.idle"_s);
};

static void startAdvancedPenetrationTask(void *pvParameters) {
    encoder.setAcceleration(0);
    u8_t lastEncoder = 0;
    String headerText = ui::strings::advancedPenetration;
    setHeader(headerText);

    while (isInCorrectState()) {
        u8_t speedValue = getAnalogAveragePercent(SampleOnPin{Pins::Remote::speedPotPin, 50});
        u8_t encoderValue = encoder.readEncoder();
        if ( abs(speedValue - currentSettings.speed.value) > 1
                    //|| speedValue == 0
                    || encoderValue != lastEncoder 
                    || currentSettings.status != currentSettings.lastStatus) {
            if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
                clearPage(true);
                currentSettings.speed.value = speedValue;
                ui::drawShape::settingBar(display.getU8g2(),"", currentSettings.speed.value, 0, 0, ui::LEFT_ALIGNED, 0, 54);

                lastEncoder = encoderValue;
                int item = floor(encoderValue / 3);
                switch (currentSettings.status) {
                    case ControlStatus::BASE_MENU:
                        item = item % BaseControls::BASE_COUNT;
                        currentSettings.baseControl = BaseControls(item);
                        break;
                    case ControlStatus::MODIFIER_MENU:
                        item = item % ModifierControls::MODIFIER_COUNT;
                        currentSettings.modifierControl = ModifierControls(item);
                        break;
                    case ControlStatus::BASE_VALUE:

                        break;
                    default:
                        break;
                }
                
                textPosition = 19;
                updateControl(currentSettings.maxDepth, currentSettings.minDepth.value);
                updateControl(currentSettings.minDepth, 0, currentSettings.maxDepth.value);
                updateControl(currentSettings.inSpeed, 1);
                updateControl(currentSettings.outSpeed, 1);
                updateControl(currentSettings.inAcceleration);
                updateControl(currentSettings.outAcceleration);
                currentSettings.changed = true;

                refreshPage(true);
                xSemaphoreGive(displayMutex);
            }
        }
        vTaskDelay(100);
    }
    vTaskDelete(nullptr);
}

static void startAdvancedPenetrationMotionTask(void *pvParameters) {
    int strokeCount = 0;
    while (isInCorrectState()) {
        if (currentSettings.speed.value == 0.0) {
            stepper->stopMove();
            vTaskDelay(100);
            continue;
        }
        if (!stepper->isRunning()) {
            strokeCount ++;
        }
        if (currentSettings.changed || !stepper->isRunning()) {
            currentSettings.changed = false;
            float speed = Config::Driver::maxSpeedMmPerSecond * (1_mm) * currentSettings.speed.getRampValue();
            int32_t targetPosition = -calibration.measuredStrokeSteps;
            if (strokeCount % 2 == 0) {
                speed = speed * currentSettings.inSpeed.getNormalizedModifiedValue(strokeCount);
                targetPosition = targetPosition * currentSettings.maxDepth.getNormalizedModifiedValue(strokeCount);
            } else {
                speed = speed * currentSettings.outSpeed.getNormalizedModifiedValue(strokeCount);
                targetPosition = targetPosition * currentSettings.minDepth.getNormalizedModifiedValue(strokeCount);
            }
            stepper->setSpeedInHz(speed);
            stepper->applySpeedAcceleration();

            u32_t distance = abs(targetPosition - stepper->getCurrentPosition());
            u32_t minAccel = speed / (distance / speed);          
            u32_t acceleration = minAccel;
            if (strokeCount % 2 == 0) {
                acceleration += minAccel * 9 * currentSettings.inAcceleration.getRampedModifiedValue(strokeCount, 0.6);
            } else {
                acceleration += minAccel * 9 * currentSettings.outAcceleration.getRampedModifiedValue(strokeCount, 0.6);
            }
            acceleration = min(acceleration, u32_t(Config::Driver::maxAcceleration * (1_mm)));
            if (acceleration > stepper->getAcceleration() || !stepper->isRunning()){
                stepper->setAcceleration(acceleration);
                stepper->applySpeedAcceleration();
            }
            if (!stepper->isRunning()) {
                stepper->moveTo(targetPosition,false);
            }
        }
        vTaskDelay(1);
    }
    vTaskDelete(nullptr);
}

void startAdvancedPenetration() {
    int stackSize = 10 * configMINIMAL_STACK_SIZE;
    currentSettings.lastStatus = ControlStatus::STATUS_COUNT;
    currentSettings.status = ControlStatus::BASE_MENU;
    encoder.setBoundaries(0,BaseControls::BASE_COUNT * 6 - 1, true);
    encoder.setEncoderValue(0);

    xTaskCreatePinnedToCore(startAdvancedPenetrationTask,
                            "startAdvancedPenetrationTask", stackSize, nullptr,
                            configMAX_PRIORITIES - 1,
                            &Tasks::runAdvancedPenetrationTaskH,
                            Tasks::operationTaskCore);

    xTaskCreatePinnedToCore(startAdvancedPenetrationMotionTask, 
                            "startAdvancedPenetrationMotionTask", stackSize, nullptr,
                            configMAX_PRIORITIES - 1, nullptr,
                            Tasks::operationTaskCore);
}
}