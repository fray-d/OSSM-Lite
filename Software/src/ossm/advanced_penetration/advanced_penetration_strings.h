#ifndef OSSM_ADVANCED_PENETRATION_STRINGS_H
#define OSSM_ADVANCED_PENETRATION_STRINGS_H

#include <Preferences.h>

#include "Progmem.h"

namespace advanced_penetration {
	std::vector<std::string> presets;
	Preferences presetStore;

	String readPreset(u8_t i) {
		presetStore.begin("presets", true);
		String output = presetStore.getString(presets[0].c_str());
		if (i > 0) {
			output += presetStore.getString(presets[i].c_str());
		}
		presetStore.end();
		return output;
	}

	void initPresets() {
		presetStore.begin("presets", true);
		//presetStore.clear();
		if (!presetStore.isKey("names")){
			presetStore.end();
			presetStore.begin("presets", false);
			presetStore.putString("names", "Simple,Teasing,Pounding,Robo,Half'n'half,Deeper,Insist,Jackhammer,Progressive,Mid,Knot (75%),Knot (50%)");
			presetStore.putString("Simple", "0:0:100,0:1:1,0:2:0,0:3:1,0:4:0,0:5:0,1:0:100,1:1:1,1:2:0,1:3:1,1:4:0,1:5:0,"
								"2:100,2:0:100,2:1:1,2:2:0,2:3:1,2:4:0,2:5:0,3:100,3:0:100,3:1:1,3:2:0,3:3:1,3:4:0,3:5:0,"
								"4:40,4:0:100,4:1:1,4:2:0,4:3:1,4:4:0,4:5:0,5:40,5:0:100,5:1:1,5:2:0,5:3:1,5:4:0,5:5:0,");
			presetStore.putString("Teasing", "2:50,");
			presetStore.putString("Pounding", "3:50,");
			presetStore.putString("Robo", "4:100,5:100,");
			presetStore.putString("Half'n'half", "0:0:50,");
			presetStore.putString("Deeper", "0:0:15,0:3:10,");
			presetStore.putString("Insist", "1:0:15,1:1:10,");
			presetStore.putString("Jackhammer", "0:0:15,0:3:9,1:0:15,1:1:9,3:0:50,3:2:8,5:0,");
			presetStore.putString("Progressive", "0:0:15,0:1:10,0:2:1,0:3:10,1:0:15,1:1:10,1:2:1,1:3:10,1:5:11,");
			presetStore.putString("Mid", "0:0:15,0:1:10,0:2:1,0:3:10,1:0:15,1:1:10,1:2:1,1:3:10,");
			presetStore.putString("Knot (75%)", "0:0:75,1:0:25,2:0:25,2:5:1,3:0:2,");
			presetStore.putString("Knot (50%)", "0:0:50,1:0:50,2:0:25,2:5:1,3:0:2,");
			presetStore.end();
			presetStore.begin("presets", true);
			ESP_LOGI("TEST","NEW STORE");
		}

		u8_t presetsCount = 0;
		std::string presetNames = presetStore.getString("names").c_str();
		for (char i : presetNames){
			if (i == ',') {
				presetsCount ++;
			}
		}
		presetsCount++;

		for (uint16_t i = 0; i < presetsCount; i ++) {
			int16_t j = presetNames.find(",");
			presets.push_back(presetNames.substr(0,j));
			if (j > 0) {
				presetNames = presetNames.substr(j+1);
			}
		}
		
		size_t space = presetStore.freeEntries();
		Serial.printf("Space: %lu, Count: %d", (unsigned long)space, presetsCount);
		presetStore.end();
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