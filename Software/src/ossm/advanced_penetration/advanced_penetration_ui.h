#ifndef OSSM_ADVANCED_PENETRATION_UI_H
#define OSSM_ADVANCED_PENETRATION_UI_H

#include "advanced_penetration_structs.h"
#include "services/encoder.h"
#include "services/display.h"
#include "Progmem.h"
#include "ui.h"
#include "utils/analog.h"

namespace advanced_penetration {
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

static void startAdvancedPenetrationUITask(void *pvParameters) {
    encoder.setAcceleration(0);
    u8_t lastEncoder = 0;
    String headerText = ui::strings::advancedPenetration;
    setHeader(headerText);

    while (stateMachine->is("advancedPenetration"_s)
                    || stateMachine->is("advancedPenetration.idle"_s)) {
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

}

#endif