#ifndef OSSM_ADVANCED_PENETRATION_STRINGS_H
#define OSSM_ADVANCED_PENETRATION_STRINGS_H

#include <Preferences.h>

#include "Progmem.h"

namespace advanced_penetration {
    std::vector<std::string> presets;
    Preferences presetStore;

    String readPreset(u8_t i) {
        presetStore.begin("presets", true);
        String output = presetStore.getString("reset");
        if (i > 0) {
            output += presetStore.getString(presets[i].c_str());
        }
        presetStore.end();
        return output;
    }

    void clearPresets() {
        presetStore.begin("presets", false);
        presetStore.clear();
        presetStore.end();
        ESP_LOGI("AP", "Clearing presets");
    }

    void savePreset(String name, String value) {
        presetStore.begin("presets", false);
        presetStore.putString(name.c_str(), value);
        presetStore.end();
        ESP_LOGI("AP", "Saving preset: %s", name.c_str());
    }

    void removePreset(String name) {
        presetStore.begin("presets", false);
        presetStore.remove(name.c_str());
        presetStore.end();
        ESP_LOGI("AP", "Deleted preset: %s", name.c_str());
    }

    bool keyExists(String key) {
        presetStore.begin("presets", true);
        bool exists = presetStore.isKey(key.c_str());
        presetStore.end();
        ESP_LOGI("AP", "%s exists: %s", key.c_str(), String(exists));
        return exists;
    }

    void factoryReset() {
        ESP_LOGI("AP", "Restoring to default presets");
        savePreset("names",
                   "Simple,Teasing,Pounding,Robo,Half'n'half,Deeper,Insist,"
                   "Jackhammer,Progressive,Mid,Knot (75%),Knot (50%),");
        savePreset("reset",
                   "0:0:100,0:1:1,0:2:0,0:3:1,0:4:0,0:5:0,1:0:100,1:1:1,1:2:0,1:3:1,"
				   "1:4:0,1:5:0,2:100,2:0:100,2:1:1,2:2:0,2:3:1,2:4:0,2:5:0,3:100,"
				   "3:0:100,3:1:1,3:2:0,3:3:1,3:4:0,3:5:0,4:40,4:0:100,4:1:1,4:2:0,"
				   "4:3:1,4:4:0,4:5:0,5:40,5:0:100,5:1:1,5:2:0,5:3:1,5:4:0,5:5:0,");
        savePreset("Simple", "");
        savePreset("Teasing", "2:50,");
        savePreset("Pounding", "3:50,");
        savePreset("Robo", "4:100,5:100,");
        savePreset("Half'n'half", "0:0:50,");
        savePreset("Deeper", "0:0:15,0:3:10,");
        savePreset("Insist", "1:0:15,1:1:10,");
        savePreset("Jackhammer", "0:0:15,0:3:9,1:0:15,1:1:9,3:0:50,3:2:8,5:0,");
        savePreset("Progressive","0:0:15,0:1:10,0:2:1,0:3:10,1:0:15,1:1:10,1:2:1,1:3:10,1:5:11,");
        savePreset("Mid", "0:0:15,0:1:10,0:2:1,0:3:10,1:0:15,1:1:10,1:2:1,1:3:10,");
        savePreset("Knot (75%)", "0:0:75,1:0:25,2:0:25,2:5:1,3:0:2,");
        savePreset("Knot (50%)", "0:0:50,1:0:50,2:0:25,2:5:1,3:0:2,");
    }

    void renamePreset(String oldName, String newName) {
        if (keyExists(oldName)) {
            savePreset(newName, presetStore.getString(oldName.c_str()));
            removePreset(oldName);
        }
        ESP_LOGI("AP", "Renamed preset: %s > %s", oldName.c_str(), newName.c_str());
    }

    std::string getPreset(String key) {
        presetStore.begin("presets", true);
        std::string preset = presetStore.getString(key.c_str()).c_str();
        presetStore.end();
        return preset;
    }

    void repopulatePresets() {
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
		ESP_LOGI("AP","Space remaining: %lu", (unsigned long)space);
	}

    void initPresets() {
        clearPresets();
        if (!keyExists("names")) {
            factoryReset();
        }
		repopulatePresets();
		updateSpace();
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