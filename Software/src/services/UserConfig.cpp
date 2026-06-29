#include "UserConfig.h"
#include "constants/Config.h"

#include <Preferences.h>

namespace UserConfig {
    float getSpeedCurve() {
        return 0.8;
    }

    std::string getDeviceName() {
        Preferences userConfig;
        userConfig.begin("UserConfig", true);
        std::string output = userConfig.getString("DeviceName","OSSM").c_str();
        userConfig.end();
        ESP_LOGD("USER CONFIG", "Name read: %s", output.c_str());
        return output;
    }

    void setDeviceName(String value) {
        Preferences userConfig;
        userConfig.begin("UserConfig", false);
        userConfig.putString("DeviceName", value);
        userConfig.end();
        ESP_LOGI("USER CONFIG", "Rename write: %s", value.c_str());
        ESP.restart();
    }

    bool getDirection() {
        Preferences userConfig;
        userConfig.begin("UserConfig", true);
        bool output = userConfig.getBool("Direction",false);
        userConfig.end();
        ESP_LOGI("USER CONFIG", "Direction read: %d", output);
        return output;
    }

    void setDirection(bool value) {
        Preferences userConfig;
        userConfig.begin("UserConfig", false);
        userConfig.putBool("Direction", value);
        userConfig.end();
        ESP_LOGI("USER CONFIG", "Direction write: %d", value);
        ESP.restart();
    }

    void reverseDirection() {
        setDirection(!getDirection());
    }

    HomingType getHomingType() {
        Preferences userConfig;
        userConfig.begin("UserConfig", true);
        HomingType output = static_cast<HomingType>(userConfig.getInt("HomingType",1));
        userConfig.end();
        ESP_LOGI("USER CONFIG","Homing type read: %d", output);
        return output;
    }

    void setHomingType(HomingType value) {
        Preferences userConfig;
        userConfig.begin("UserConfig", false);
        userConfig.putInt("HomingType", value);
        userConfig.end();
        ESP_LOGI("USER CONFIG", "Homing type write: %d", value);
    }

    float getRailLength() {
        Preferences userConfig;
        userConfig.begin("UserConfig", true);
        float output = userConfig.getFloat("RailLength",180.0);
        userConfig.end();
        ESP_LOGI("USER CONFIG","Rail length read: %f", output);
        return output;
    }

    void setRailLength(float value) {
        Preferences userConfig;
        userConfig.begin("UserConfig", false);
        
        userConfig.putFloat("RailLength", constrain(value,Config::Driver::minStrokeLengthMm,Config::Driver::maxStrokeMm));
        userConfig.end();
        ESP_LOGI("USER CONFIG", "Rail length write: %d", value);
    }

    bool getReHome() {
        Preferences userConfig;
        userConfig.begin("UserConfig", true);
        bool output = userConfig.getBool("ReHome",true);
        userConfig.end();
        ESP_LOGI("USER CONFIG", "Home between moodes read: %d", output);
        return output;
    }

    void setReHome(bool value) {
        Preferences userConfig;
        userConfig.begin("UserConfig", false);
        userConfig.putBool("ReHome", value);
        userConfig.end();
        ESP_LOGI("USER CONFIG", "Home between modes write: %d", value);
    }
}
