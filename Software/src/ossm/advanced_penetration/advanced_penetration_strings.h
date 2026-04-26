#ifndef OSSM_ADVANCED_PENETRATION_STRINGS_H
#define OSSM_ADVANCED_PENETRATION_STRINGS_H

#include <Preferences.h>

#include "Progmem.h"

namespace advanced_penetration {
    std::vector<std::string> presets;
    Preferences presetStore;

    String readPresetValueCommand(u8_t i) {
        presetStore.begin("presets", true);
        String output = presetStore.getString("reset");
        if (i > 0) {
            output += presetStore.getString(presets[i].c_str());
        }
        presetStore.end();
        return output;
    }

    String readPresetValueCommand(String name) {
        presetStore.begin("presets", true);
        String output = presetStore.getString("reset");
        output += presetStore.getString(name.c_str());
        presetStore.end();
        return output;
    }

    void clearPresets() {
        presetStore.begin("presets", false);
        presetStore.clear();
        presetStore.end();
        ESP_LOGI("AP", "Clearing presets");
    }

    void saveKey(String name, String value) {
        presetStore.begin("presets", false);
        presetStore.putString(name.c_str(), value);
        presetStore.end();
        ESP_LOGI("AP", "Saving key: %s", name.c_str());
    }

    void saveNames() {
        std::string names;
        for (u8_t i = 0; i < presets.size(); i++) {
            names += presets[i] + ",";
        }
        ESP_LOGI("AP", "Saving names: %s", names.c_str());
        saveKey("names", names.c_str());
    }

    bool keyExists(String key) {
        presetStore.begin("presets", true);
        bool exists = presetStore.isKey(key.c_str());
        presetStore.end();
        ESP_LOGI("AP", "%s exists: %s", key.c_str(), String(exists));
        return exists;
    }

    String generateName() {
        u8_t i = 1;
        String key = "User Pattern " + String(i);
        while (keyExists(key)) {
            i++;
            key = "User Pattern " + String(i);
        }
        return key;
    }

    void savePreset(String name, String value) {
        if (name.length() == 0) {
            name = generateName();
        }
        saveKey(name, value);
        if (std::find(presets.begin(), presets.end(), name.c_str()) == presets.end()) {
            presets.push_back(name.c_str());
            saveNames();
        }
        ESP_LOGI("AP", "Saving preset: %s", name.c_str());
    }

    void deletePreset(String name) {
        presetStore.begin("presets", false);
        presetStore.remove(name.c_str());
        presetStore.end();
        presets.erase(remove(presets.begin(), presets.end(), name.c_str()), presets.end());
        saveNames();
        ESP_LOGI("AP", "Deleted preset: %s", name.c_str());
    }

    void factoryReset() {
        clearPresets();
        ESP_LOGI("AP", "Restoring to default presets");
        saveKey("names",
                "Simple,Teasing,Pounding,Robo,Half'n'half,Deeper,Insist,"
                "Jackhammer,Progressive,Mid,Knot (75%),Knot (50%),");
        saveKey("reset",
                "0:0:100,0:1:1,0:2:0,0:3:1,0:4:0,0:5:0,1:0:100,1:1:1,1:2:0,1:3:1,"
                "1:4:0,1:5:0,2:100,2:0:100,2:1:1,2:2:0,2:3:1,2:4:0,2:5:0,3:100,"
                "3:0:100,3:1:1,3:2:0,3:3:1,3:4:0,3:5:0,4:40,4:0:100,4:1:1,4:2:0,"
                "4:3:1,4:4:0,4:5:0,5:40,5:0:100,5:1:1,5:2:0,5:3:1,5:4:0,5:5:0,");
        saveKey("Simple", "");
        saveKey("Teasing", "2:50,");
        saveKey("Pounding", "3:50,");
        saveKey("Robo", "4:100,5:100,");
        saveKey("Half'n'half", "0:0:50,");
        saveKey("Deeper", "0:0:15,0:3:10,");
        saveKey("Insist", "1:0:15,1:1:10,");
        saveKey("Jackhammer", "0:0:15,0:3:9,1:0:15,1:1:9,3:0:50,3:2:8,5:0,");
        saveKey("Progressive", "0:0:15,0:1:10,0:2:1,0:3:10,1:0:15,1:1:10,1:2:1,1:3:10,1:5:11,");
        saveKey("Mid", "0:0:15,0:1:10,0:2:1,0:3:10,1:0:15,1:1:10,1:2:1,1:3:10,");
        saveKey("Knot (75%)", "0:0:75,1:0:25,2:0:25,2:5:1,3:0:2,");
        saveKey("Knot (50%)", "0:0:50,1:0:50,2:0:25,2:5:1,3:0:2,");
    }

    std::string getPreset(String key) {
        presetStore.begin("presets", true);
        std::string preset = presetStore.getString(key.c_str()).c_str();
        presetStore.end();
        return preset;
    }

    void repopulatePresets() {
        presets.clear();
        std::string presetNames = getPreset("names");
        int16_t j = presetNames.find(",");
        while (j > 0) {
            presets.push_back(presetNames.substr(0, j));
            if (j > 0) {
                presetNames = presetNames.substr(j + 1);
            }
            j = presetNames.find(",");
        }
    }

    void updateSpace() {
        size_t space = presetStore.freeEntries();
        ESP_LOGI("AP", "Space remaining: %lu", (unsigned long)space);
    }

    void initPresets() {
        // clearPresets();
        if (!keyExists("names")) {
            factoryReset();
        }
        updateSpace();
        repopulatePresets();
    }

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
}

#endif