#include "UserConfig.h"

#include <Preferences.h>

namespace UserConfig {
    std::string deviceName = "";
    float railLength = -1;
    float maxAcceleration = -1;
    float maxRPM = -1;
    float motorStepPerRevolution = -1;
    float pulleyToothCount = -1;
    float beltPitchMm = -1;
    float stepsPerMM = -1;
    float maxSpeedMMS = -1;
    float homingSpeed = -1;
    float sensorLimit = -1;
    float speedCurve = -1;

    float readNVSFloat(char* key, float def) {
        Preferences userConfig;
        userConfig.begin("UserConfig", true);
        float value = userConfig.getFloat(key, def);
        userConfig.end();
        ESP_LOGI("USER CONFIG","%s read: %f", key, value);
        return value;
    }

    void writeNVSFloat(char* key, float value) {
        Preferences userConfig;
        userConfig.begin("UserConfig", false);
        userConfig.putFloat(key, value);
        userConfig.end();
        ESP_LOGI("USER CONFIG", "%s write: %f", key, value);
    }

    float getSpeedCurve() {
        if (speedCurve < 0) {
            speedCurve = readNVSFloat("SpeedCurve", 0.8);
        }
        return speedCurve;
    }
    void setSpeedCurve(float value) {
        value = constrain(value, 0.5, 1.0);
        writeNVSFloat("SpeedCurve", value);
        speedCurve = value;
    }

    float getSensorLimit() {
        if (sensorLimit < 0) {
            sensorLimit = readNVSFloat("SensorLimit", 2.5);
        }
        return sensorLimit;
    }
    void setSensorLimit(float value){
        value = constrain(value, 1.5, 5.0);
        writeNVSFloat("SensorLimit", value);
        sensorLimit = value;
    }

    std::string getDeviceName() {
        if (deviceName.empty()) {
            Preferences userConfig;
            userConfig.begin("UserConfig", true);
            deviceName = userConfig.getString("DeviceName","OSSM").c_str();
            userConfig.end();
            ESP_LOGD("USER CONFIG", "Name read: %s", deviceName.c_str());
        }
        return deviceName;
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
        if (railLength < 0) {
            railLength = readNVSFloat("RailLength",180.0);
        }
        return railLength;
    }
    void setRailLength(float value) {
        value = constrain(value, getMinRailLength(), getMaxRailLength());
        writeNVSFloat("RailLength", value);
        railLength = value;
    }

    float getMaxAcceleration() {
        if (maxAcceleration < 0) {
            maxAcceleration = readNVSFloat("MaxAcceleration",50000.0);
        }
        return maxAcceleration;
    }
    void setMaxAcceleration(float value) {
        writeNVSFloat("MaxAcceleration", value);
        maxAcceleration = value;
    }

    float getMotorRPM() {
        if (maxRPM < 0) {
            maxRPM = readNVSFloat("MaxRPM",1500.0);
        }
        return maxRPM;
    }
    void setMotorRPM(float value) {
        writeNVSFloat("MaxRPM", value);
        maxRPM = value;
        setMaxSpeedMMS();
    }

    float getMotorStepsPR() {
        if (motorStepPerRevolution < 0) {
            motorStepPerRevolution = readNVSFloat("MotorSteps", 800.0);
        }
        return motorStepPerRevolution;
    }
    void setMotorStepsPR(float value) {
        writeNVSFloat("MotorSteps", value);
        motorStepPerRevolution = value;
        setStepsPerMM();
    }

    float getPulleyTeeth(){
        if (pulleyToothCount < 0) {
            pulleyToothCount = readNVSFloat("PulleyTeeth",20.0);
        }
        return pulleyToothCount;
    }
    void setPulleyTeeth(float value){
        writeNVSFloat("PulleyTeeth", value);
        pulleyToothCount = value;
        setMaxSpeedMMS();
        setStepsPerMM();
    }

    float getBeltPitch(){
        if (beltPitchMm < 0) {
            beltPitchMm = readNVSFloat("BeltPitch",2.0);
        }
        return beltPitchMm;
    }
    void setBeltPitch(float value){
        writeNVSFloat("BeltPitch", value);
        beltPitchMm = value;
        setMaxSpeedMMS();
        setStepsPerMM();
    }

    float getStepsPerMM(float value){
        if (stepsPerMM < 0) {
            setStepsPerMM();
        }
        return stepsPerMM * value;
    }
    void setStepsPerMM() {
        //cache this
        stepsPerMM = (getMotorStepsPR() / (getPulleyTeeth() * getBeltPitch()));
    }

    float getMaxSpeedMMS() {
        if (maxSpeedMMS < 0) {
            setMaxSpeedMMS();
        }
        return maxSpeedMMS;
    }

    void setMaxSpeedMMS(){
        //cache this
        maxSpeedMMS = getMotorRPM() / 60.0 * getPulleyTeeth() * getBeltPitch();
    }

    float getMaxRailLength(){
        float rail = getRailLength();
        if (rail > 500.0) {
            return rail;
        }
        return 500.0;
    }

    float getMinRailLength() {
        return 50.0;
    }

    float getHomingSpeed(){
        if (homingSpeed < 0) {
            homingSpeed = readNVSFloat("HomingSpeed",25);
        }
        return homingSpeed;
    }
    void setHomingSpeed(float value){
        value = constrain(value,10,40);
        writeNVSFloat("HomingSpeed", value);
        homingSpeed = value;
    }
}
