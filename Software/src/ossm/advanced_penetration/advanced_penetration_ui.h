#ifndef OSSM_ADVANCED_PENETRATION_UI_H
#define OSSM_ADVANCED_PENETRATION_UI_H

#include "Progmem.h"
#include "advanced_penetration_structs.h"
#include "components/HeaderBar.h"
#include "services/display.h"
#include "services/encoder.h"
#include "ui.h"
#include "utils/analog.h"

namespace advanced_penetration {

    u8_t textPosition = 19;

    void selectText(u8_t index) { 
        display.drawFrame(111, 10 + 9 * index, 17, 11);
    }

    void drawRightHeader(String text) {
        display.setFont(ui::Font::header);
        uint16_t stringWidth = display.getUTF8Width(text.c_str());
        display.drawUTF8(127 - stringWidth, 8, text.c_str());
    }

    void drawText(String controlText, bool centered = true) {
        uint16_t stringWidth = display.getUTF8Width(controlText.c_str());
        if (centered) {
            if (controlText == "100") {
                display.drawUTF8(119 - stringWidth / 2, textPosition, controlText.c_str());
            } else {
                display.drawUTF8(120 - stringWidth / 2, textPosition, controlText.c_str());
            }
        } else {
            display.drawUTF8(125 - stringWidth, textPosition, controlText.c_str());
        }
        textPosition += 9;
    }

    void setEncoderIfStatus(Control& c, ControlStatus status) {
        if (currentSettings.status == status) {
            display.setFont(ui::Font::header);
            if (currentSettings.lastStatus != currentSettings.status) {
                encoder.setBoundaries(c.minValue, c.maxValue, false);
                encoder.setEncoderValue(c.value - 1);
            }
            c.value = constrain(encoder.readEncoder(), c.minValue, c.maxValue);
            currentSettings.changed = true;
        }
        currentSettings.lastStatus = currentSettings.status;
    }

    void drawModifierControl(ModifierControl& m) {
        display.setFont(ui::Font::header);
        if (currentSettings.modifierControl == m.id) {
            drawRightHeader(m.name);
            setEncoderIfStatus(m, ControlStatus::MODIFIER_VALUE);
            if (currentSettings.status == ControlStatus::MODIFIER_VALUE) {
                ui::drawShape::settingBar(display.getU8g2(), "", m.value, 115, 0, ui::LEFT_ALIGNED, 0, 54, m.minValue, m.maxValue);
                return;
            }
        }
        if (currentSettings.status == ControlStatus::MODIFIER_MENU) {
            drawText(String(m.value));
        }
    }

    StrokeMath calculateStroke(u8_t speed, float accelValue) {
        StrokeMath stroke;
        float minAccel = speed / (currentSettings.usableDepth() / float(speed));
        stroke.accel = minAccel + (minAccel * 9) * accelValue;
        stroke.accelTime = speed / stroke.accel;
        float accelDistance = 0.5 * stroke.accel * pow(stroke.accelTime, 2);
        float maxSpeedDistance = currentSettings.usableDepth() - accelDistance * 2;
        float maxSpeedTime = maxSpeedDistance / speed;
        stroke.totalTime = stroke.accelTime * 2 + maxSpeedTime;
        return stroke;
    }

    float bezierMath(float v0, float v1, float v2, float v3, float t) {
        float output = pow(1 - t, 3) * v0;
        output += 3 * pow(1 - t, 2) * t * v1;
        output += 3 * (1 - t) * pow(t, 2) * v2;
        output += pow(t, 3) * v3;
        return output;
    }

    void bezCurve(u8_t x0, u8_t x1, u8_t y0, u8_t y1, float r, float t) {
        float diff = (x1 - x0) * r;
        u8_t x = floor(bezierMath(x0, x0 + diff, x1 - diff, x1, t));
        u8_t y = floor(bezierMath(y0, y0, y1, y1, t));
        display.drawPixel(x, y);
    }

    void drawStroke(bool modified = false) {
        StrokeMath inStroke, outStroke;
        u8_t x0, x1, x2, y0, y1, w, h, split, spacing;
        float exp = 0.6;
        h = 54;
        if (modified) {
            inStroke = calculateStroke(currentSettings.inSpeed.getModifiedValue(), currentSettings.inAcceleration.getRampedModifiedValue(exp));
            outStroke = calculateStroke(currentSettings.outSpeed.getModifiedValue(), currentSettings.outAcceleration.getRampedModifiedValue(exp));
            y0 = display.getHeight() - (currentSettings.minDepth.getNormalizedModifiedValue() * h);
            y1 = display.getHeight() - (currentSettings.maxDepth.getNormalizedModifiedValue() * h);
            spacing = 3;
        } else {
            inStroke = calculateStroke(currentSettings.inSpeed.value, currentSettings.inAcceleration.getRampValue(exp));
            outStroke = calculateStroke(currentSettings.outSpeed.value, currentSettings.outAcceleration.getRampValue(exp));
            y0 = display.getHeight() - (currentSettings.minDepth.getNormalized() * h);
            y1 = display.getHeight() - (currentSettings.maxDepth.getNormalized() * h);
            spacing = 1;
        }
        w = 108;
        split = w * (inStroke.totalTime / (inStroke.totalTime + outStroke.totalTime));
        x0 = 10;
        x1 = x0 + split;
        x2 = x0 + w;
        for (u8_t p = 0; p <= split; p += spacing) {
            bezCurve(x0, x1, y0, y1, inStroke.accelTime / inStroke.totalTime, p / float(split));
        }
        for (u8_t p = 0; p <= (w - split); p += spacing) {
            bezCurve(x1 - 1, x2, y1, y0, outStroke.accelTime / outStroke.totalTime, p / float(w - split));
        }
    }

    void drawBars(BaseControl& a){
        u8_t steps = a.modifier->stepCount();
        if (steps == 0) {
            return;
        }
        u8_t barWdith = 101 / steps;
        u8_t w = barWdith * steps;
        u8_t x = (101 - w) / 2 + 10;
        for (int step = 0; step < steps; step++) {
            u8_t value = 54 * a.modifier->getModification(step);
            if (a.modifier->getControlForStep(step) == currentSettings.modifierControl) {
                display.drawBox(x, display.getHeight() - value, barWdith + 1, value);
            } else {
                display.drawFrame(x, display.getHeight() - value, barWdith + 1, value);
            }
            x += barWdith;
        }
    }

    void drawModifier(BaseControl& a) {
        if (a.modifier == nullptr) {
            a.modifier = new Modifier;
        }
        if (a.modifier->amplitude.value == 100) {
            if (int(currentSettings.modifierControl) % 2 == 0) {
                currentSettings.modifierControl = ModifierControls::AMPLITUDE;
            } else {
                currentSettings.modifierControl = ModifierControls::GO_BACK;
            }
        }
        drawModifierControl(a.modifier->amplitude);
        if (a.modifier->amplitude.value < 100) {
            drawModifierControl(a.modifier->inStep);
            drawModifierControl(a.modifier->inWait);
            drawModifierControl(a.modifier->outStep);
            drawModifierControl(a.modifier->outWait);
            drawModifierControl(a.modifier->offset);
        }
        switch (currentSettings.modifierControl) {
            case ModifierControls::GO_BACK:
                drawRightHeader("Go Back");
                display.drawFrame(88, 0, 40, 11);
                //intentionally fall through
            case ModifierControls::AMPLITUDE:
                drawStroke();
                drawStroke(true);
                break;
            default:
                drawBars(a);
        }
        if (currentSettings.status == ControlStatus::MODIFIER_MENU) {
            selectText(currentSettings.modifierControl);
        }
    }

    void updateControl(BaseControl& a, u8_t minVal = 0, u8_t maxVal = 100) {
        display.setFont(ui::Font::header);
        if (currentSettings.status == ControlStatus::MODIFIER_MENU || currentSettings.status == ControlStatus::MODIFIER_VALUE) {
            if (currentSettings.baseControl == a.id) {
                drawModifier(a);
            }
            return;
        }
        a.minValue = minVal;
        a.maxValue = maxVal;
        if (currentSettings.baseControl == a.id) {
            setEncoderIfStatus(a, ControlStatus::BASE_VALUE);
            drawStroke();
            drawStroke(true);
        }

        if (encoder.readEncoder() / 3 >= BaseControls::BASE_COUNT && currentSettings.status == ControlStatus::BASE_MENU) {
            display.setFont(ui::Font::header);

            if (a.modifier != nullptr && a.modifier->active()) {
                drawText(String(a.modifier->amplitude.value));
            } else {
                drawText("Off");
            }
            if (currentSettings.baseControl == a.id) {
                selectText(currentSettings.baseControl);
                drawRightHeader(a.name + " Mod");
            }            return;
        }
        if (currentSettings.status == ControlStatus::BASE_MENU) {
            if (currentSettings.baseControl == a.id) {
                selectText(currentSettings.baseControl);
                drawRightHeader(a.name);
            }
            drawText(String(a.value));
            return;
        }

        if (currentSettings.baseControl == a.id) {
            ui::drawShape::settingBar(display.getU8g2(), "", a.value, 118, 0, ui::LEFT_ALIGNED, 0, 54);
            drawRightHeader(a.name);
        }
    }

    void handlePreset() {
        u8_t selectedPreset = encoder.readEncoder() / 3;
        if (selectedPreset < presets.size()) {
            currentSettings.processStringCommand(readPresetValueCommand(encoder.readEncoder() / 3));
        } else {
            savePreset("", currentSettings.getPreset());
        }
        stateMachine->process_event(Done{});
        currentSettings.status = ControlStatus::BASE_MENU;
    }

    void advancedClick() {
        u8_t c = 100;
        bool loop = true;
        u8_t value = 0;
        if (stateMachine->is("advancedPenetration.presets"_s)) {
            handlePreset();
            return;
        } 
        switch (currentSettings.status) {
            case ControlStatus::BASE_MENU:
                if (encoder.readEncoder() / 3 >= BaseControls::BASE_COUNT) {
                    currentSettings.status = ControlStatus::MODIFIER_MENU;
                    c = ModifierControls::MODIFIER_COUNT * 3;
                    value = (ModifierControls::MODIFIER_COUNT - 1) * 3;
                } else {
                    currentSettings.status = ControlStatus::BASE_VALUE;
                    loop = false;
                }
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
                currentSettings.status = ControlStatus::BASE_MENU;
                value = currentSettings.baseControl * 3;
                c = BaseControls::BASE_COUNT * 6;
                break;
        }
        encoder.setBoundaries(0, c - 1, loop);
        encoder.setEncoderValue(value);
    }

    void drawPresetMenu(u8_t item) {
        const char* convert[presets.size() + 1];
        for (u8_t i = 0; i < presets.size(); i++) {
            convert[i] = presets[i].c_str();
        }
        convert[presets.size()] = "Save New Preset";
        ui::MenuData data{};
        data.items = convert;
        data.numItems = presets.size() + 1;
        encoder.setBoundaries(0, data.numItems * 3 - 1, true);
        if (item > data.numItems) {
            item = data.numItems - 1;
        }
        data.selectedIndex = item;
        ui::drawMenu(display.getU8g2(), data);
        currentSettings.lastStatus = currentSettings.status;
    }

    void drawMainUI(u8_t item) {
        switch (currentSettings.status) {
            case ControlStatus::BASE_MENU:
                item = item % BaseControls::BASE_COUNT;
                currentSettings.baseControl = BaseControls(item);
                break;
            case ControlStatus::MODIFIER_MENU:
                item = item % ModifierControls::MODIFIER_COUNT;
                currentSettings.modifierControl = ModifierControls(item);
                break;
            default:
                break;
        }

        textPosition = 19;
        updateControl(currentSettings.maxDepth, currentSettings.minDepth.value + 1);
        updateControl(currentSettings.minDepth, 0, currentSettings.maxDepth.value - 1);
        updateControl(currentSettings.inSpeed, 1);
        updateControl(currentSettings.outSpeed, 1);
        updateControl(currentSettings.inAcceleration);
        updateControl(currentSettings.outAcceleration);
    }

    void handleDrawPage(u8_t speedValue, u8_t encoderValue) {
        if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
            ui::clearPage(display.getU8g2(), true, true);
            ui::setHeader(display.getU8g2(),sp);
            currentSettings.speed.value = speedValue;
            ui::drawShape::settingBar(display.getU8g2(), "",
                                      currentSettings.speed.value, 0, 0,
                                      ui::LEFT_ALIGNED, 0, 54);

            if (stateMachine->is("advancedPenetration.presets"_s)) {
                drawPresetMenu(floor(encoderValue / 3));
            } else {
                drawMainUI(floor(encoderValue / 3));
            }
            refreshPage(true, true);
            xSemaphoreGive(displayMutex);
        }
    }

    static void startAdvancedPenetrationUITask(void* pvParameters) {
        encoder.setAcceleration(0);
        u8_t lastEncoder = 0;
        showHeaderIcons = false;

        while (stateMachine->is("advancedPenetration"_s) || stateMachine->is("advancedPenetration.idle"_s) ||
               stateMachine->is("advancedPenetration.presets"_s)) {
            if (stateMachine->is("advancedPenetration.presets"_s) && currentSettings.status != ControlStatus::STATUS_COUNT) {
                currentSettings.status = ControlStatus::STATUS_COUNT;
            }
            u8_t speedValue = getAnalogAveragePercent(SampleOnPin{Pins::Remote::speedPotPin, 50});
            u8_t encoderValue = encoder.readEncoder();
            bool speedChange = abs(speedValue - currentSettings.speed.value) > 1 || (speedValue == 0 && currentSettings.speed.value != 0);
            if (speedChange) {
                currentSettings.changed = true;
            }
            if (speedChange || encoderValue != lastEncoder || currentSettings.status != currentSettings.lastStatus) {
                handleDrawPage(speedValue, encoderValue);
                lastEncoder = encoderValue;
            }
            vTaskDelay(100);
        }
        vTaskDelete(nullptr);
    }

}

#endif