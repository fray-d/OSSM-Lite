const LEGACY_OSSM_UUID = "522b443a-4f53-534d-0001-420badbabe69";
const OSSM_UUID = "4f53534d-0000-0000-0000-000000000000";
const HOMING_TYPE_UUID = "4f53534d-436f-6e66-6967-486f6d696e67";
const RAIL_LENGTH_UUID = "4f53534d-436f-6e66-6967-4c656e677468";
const HOME_BETWEEN_MODES_UUID = "4f53534d-436f-6e66-6967-5265486f6d65";
const REVERSE_RAIL_UUID = "4f53534d-436f-6e66-6967-496e76657274";
const DEVICE_NAME_UUID = "4f53534d-436f-6e66-6967-52656e616d65";
const WIFI_UUID = "4f53534d-436f-6e66-6967-202057694669";
const UPDATE_UUID = "4f53534d-436f-6e66-6967-557064617465";
const MACCEL_UUID = "4f53534d-436f-6e66-6967-4d414343454c";
const MAXRPM_UUID = "4f53534d-436f-6e66-6967-4d415852504d";
const STEPPR_UUID = "4f53534d-436f-6e66-6967-535445505052";
const PULLEY_UUID = "4f53534d-436f-6e66-6967-50554c4c4559";
const BPITCH_UUID = "4f53534d-436f-6e66-6967-425049544348";

//AP Mode
const PRESETS_UUID = "4f53534d-6164-7661-6e63-656470727374"
const STATUS_UUID = "4f53534d-6164-7661-6e63-656473746174"
const CONTROL_UUID = "4f53534d-6164-7661-6e63-6564636f6e74"

const encoder = new TextEncoder();
const decoder = new TextDecoder();

function clearCheck(element) {
    element.removeAttribute('aria-invalid');
}

var serverRef, serviceRef
async function handleConnect() {
    var connectButton = document.getElementById("connect");
    connectButton.ariaBusy = true;
    connectButton.ariaDisabled = true;
    try {
        const bleDevice = await navigator.bluetooth.requestDevice({
            filters:[{services:[OSSM_UUID]}]
        });
        bleDevice.addEventListener('gattserverdisconnected', handleDisconnect);
        console.log("Device Found");

        serverRef = await bleDevice.gatt.connect();
        console.log("Connected to server");
        
        serviceRef = await serverRef.getPrimaryService(OSSM_UUID);
        console.log("Got OSSM service");

        connectButton.style.display = 'none';
        document.getElementById("controls").style.display = "block";
    } catch (e){
        console.log("Error: " + e);
    }
    connectButton.ariaBusy = false;
    connectButton.ariaDisabled = false;
}

function handleDisconnect(){
    document.getElementById("connect").style.display = 'block';
    document.getElementById("controls").style.display = "none";
}

function firmwareWarning(){
    serverRef.device.gatt.disconnect();
    document.getElementById("oldFirmware").open = true;
}
function firmareClose() {
    document.getElementById("oldFirmware").open = false;
}

var accelRef, accelElement;
async function initMaxAcceleration() {
    accelElement = document.getElementById("maxAcceleration");
    try{
        accelRef = await serviceRef.getCharacteristic(MACCEL_UUID);
        console.log("Accel characteristic connected");
        readMaxAcceleration();
    } catch {
        firmwareWarning();
    }
}
async function readMaxAcceleration() {
    var value = await accelRef.readValue();
    value = decoder.decode(value);
    console.log("Max Acceleration: " + value);
    accelElement.value = value;
    accelElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(accelElement);},2000);
}
async function writeMaxAcceleration() {
    var value = accelElement.value;
    console.log("Write max acceleration: " + value);
    value = encoder.encode(value);
    accelRef.writeValueWithoutResponse(value);
    readMaxAcceleration();
}

var rpmRef, rpmElement;
async function initMotorRPM() {
    rpmElement = document.getElementById("motorRPM");
    try{
        rpmRef = await serviceRef.getCharacteristic(MAXRPM_UUID);
        console.log("RPM characteristic connected");
        readMotorRPM();
    } catch {
        firmwareWarning();
    }
}
async function readMotorRPM() {
    var value = await rpmRef.readValue();
    value = decoder.decode(value);
    console.log("RPM: " + value);
    rpmElement.value = value;
    rpmElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(rpmElement);},2000);
}
async function writeMotorRPM() {
    var value = rpmElement.value;
    console.log("Write RPM: " + value);
    value = encoder.encode(value);
    rpmRef.writeValueWithoutResponse(value);
    readMotorRPM();
}

var stepsRef, stepsElement;
async function initStepsPerRevolution() {
    stepsElement = document.getElementById("stepsPerRevolution");
    try{
        stepsRef = await serviceRef.getCharacteristic(STEPPR_UUID);
        console.log("Steps per revolution characteristic connected");
        readStepsPerRevolution();
    } catch {
        firmwareWarning();
    }
}
async function readStepsPerRevolution() {
    var value = await stepsRef.readValue();
    value = decoder.decode(value);
    console.log("Steps per revolution:" + value);
    stepsElement.value = value;
    stepsElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(stepsElement);},2000);
}
async function writeStepsPerRevolution() {
    var value = stepsElement.value;
    console.log("Write steps per revolution: " + value);
    value = encoder.encode(value);
    stepsRef.writeValueWithoutResponse(value);
    readStepsPerRevolution();
}

var pulleyRef, pulleyElement;
async function initPulleyTeeth() {
    pulleyElement = document.getElementById("pulleyTeeth");
    try{
        pulleyRef = await serviceRef.getCharacteristic(PULLEY_UUID);
        console.log("Pulley teeth characteristic connected");
        readPulleyTeeth();
    } catch {
        firmwareWarning();
    }
}
async function readPulleyTeeth() {
    var value = await pulleyRef.readValue();
    value = decoder.decode(value);
    console.log("Pulley teeth:" + value);
    pulleyElement.value = value;
    pulleyElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(pulleyElement);},2000);
}
async function writePulleyTeeth() {
    var value = pulleyElement.value;
    console.log("Write pulley teeth: " + value);
    value = encoder.encode(value);
    pulleyRef.writeValueWithoutResponse(value);
    readPulleyTeeth();
}

var beltPitchRef, beltPitchElement;
async function initBeltPitch() {
    beltPitchElement = document.getElementById("beltPitch");
    try{
        beltPitchRef = await serviceRef.getCharacteristic(BPITCH_UUID);
        console.log("Belt pitch characteristic connected");
        readBeltPitch();
    } catch {
        firmwareWarning();
    }
}
async function readBeltPitch() {
    var value = await beltPitchRef.readValue();
    value = decoder.decode(value);
    console.log("Belt Pitch: " + value);
    beltPitchElement.value = value;
    beltPitchElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(beltPitchElement);},2000);
}
async function writeBeltPitch() {
    var value = beltPitchElement.value;
    console.log("Write belt pitch: " + value);
    value = encoder.encode(value);
    beltPitchRef.writeValueWithoutResponse(value);
    readBeltPitch();
}

var homingTypeRef, homingElement;
async function initHomingType() {
    homingElement = document.getElementById("homingType");
    try{
        homingTypeRef = await serviceRef.getCharacteristic(HOMING_TYPE_UUID);
        console.log("Homing characteristic connected");
        readHomingType();
    } catch {
        firmwareWarning();
    }
}
async function readHomingType() {
    var value = await homingTypeRef.readValue();
    value = decoder.decode(value);
    console.log("Homing Type: " + value);
    homingElement.value = value;
    homingElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(homingElement);},2000);
}
async function writeHomingType() {
    var value = homingElement.value;
    console.log("Write homing type: " + value);
    value = encoder.encode(value);
    homingTypeRef.writeValueWithoutResponse(value);
    readHomingType();
}

var railLengthRef, railLengthElement;
async function initRailLength() {
    railLengthElement = document.getElementById("railLength");
    try{
        railLengthRef = await serviceRef.getCharacteristic(RAIL_LENGTH_UUID);
        console.log("Rail Length characteristic connected");
        readRailLength();
    } catch {
        firmwareWarning();
    }
}
async function readRailLength() {
    var value = await railLengthRef.readValue();
    value = decoder.decode(value);
    console.log("Rail Length: " + value);
    railLengthElement.value = value;
    railLengthElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(railLengthElement);},2000);
}
async function writeRailLength() {
    var value = railLengthElement.value;
    console.log("Write Rail Length: " + value);
    value = encoder.encode(value);
    railLengthRef.writeValueWithoutResponse(value);
    readRailLength();
}

var homeBetweenRef, homeBetweenElement;
async function initHomeBetween() {
    homeBetweenElement = document.getElementById("homeBetweenModes");
    try{
        homeBetweenRef = await serviceRef.getCharacteristic(HOME_BETWEEN_MODES_UUID);
        console.log("Home between modes characteristic connected");
        readHomeBetween();
    } catch {
        firmwareWarning();
    }
}
async function readHomeBetween() {
    var value = await homeBetweenRef.readValue();
    value = decoder.decode(value);
    console.log("Homing Between Modes: " + value);
    homeBetweenElement.checked = value.toLowerCase() === "true";
    homeBetweenElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(homeBetweenElement);},2000);
}
async function writeHomeBetwee() {
    var value = homeBetweenElement.checked;
    console.log("Write homing between: " + value);
    value = encoder.encode(value);
    homeBetweenRef.writeValueWithoutResponse(value);
    readHomeBetween()
}

var reverseRailRef, reverseRailElement;
async function initReverseRail() {
    reverseRailElement = document.getElementById("reverseRail");
    try{
        reverseRailRef = await serviceRef.getCharacteristic(REVERSE_RAIL_UUID);
        console.log("Reverse rail characteristic connected");
        readReverseRail();
    } catch {
        firmwareWarning();
    }
}
async function readReverseRail() {
    var value = await reverseRailRef.readValue();
    value = decoder.decode(value);
    console.log("Reverse rail: " + value);
    reverseRailElement.checked = value.toLowerCase() === "true";
    reverseRailElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(reverseRailElement);},2000);
}
async function writeReverseRail() {
    var value = reverseRailElement.checked;
    console.log("Write reverse rail: " + value);
    value = encoder.encode(value);
    reverseRailRef.writeValueWithoutResponse(value);
}

var deviceNameRef, deviceNameElement;
async function initDeviceName() {
    deviceNameElement = document.getElementById("deviceName");
    try{
        deviceNameRef = await serviceRef.getCharacteristic(DEVICE_NAME_UUID);
        console.log("Device name characteristic connected");
        readDeviceName();
    } catch {
        firmwareWarning();
    }
}
async function readDeviceName() {
    var value = await deviceNameRef.readValue();
    value = decoder.decode(value);
    console.log("Device Name: " + value);
    deviceNameElement.value = value;
    deviceNameElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(deviceNameElement);},2000);
}
async function writeDeviceName() {
    var value = deviceNameElement.value;
    console.log("Write Device Name: " + value);
    value = encoder.encode(value);
    deviceNameRef.writeValueWithoutResponse(value);
}

var wifiRef, ssidElement, passwordElement;
async function initWiFi() {
    ssidElement = document.getElementById("SSID");
    passwordElement = document.getElementById("Password");
    try{
        wifiRef = await serviceRef.getCharacteristic(WIFI_UUID);
        console.log("WiFi characteristic connected");
        readWiFi();
    } catch {
        firmwareWarning();
    }
}
async function readWiFi() {
    document.getElementById("Update").style.display = 'none';
    var value = await wifiRef.readValue();
    value = decoder.decode(value);
    value = JSON.parse(value)
    ssidElement.value = value['ssid'];
    ssidElement.ariaInvalid = !value['connected'];
    if (value['connected']) {
        document.getElementById("Update").style.display = 'block';
    }
    passwordElement.value = "";
    console.log(value['connected']);
    console.log("WiFi: %o", value);
}
async function writeWiFi() {
    var ssid = ssidElement.value;
    var password = passwordElement.value;
    var value = "set:wifi:" + ssid + "|" + password;
    console.log("Write WiFi: " + ssid);
    value = encoder.encode(value);
    await wifiRef.writeValue(value);
    readWiFi();
}

var presetRef, presetListElement;
async function initPresets() {
    presetListElement = document.getElementById("presetList");
    try{
        presetRef = await serviceRef.getCharacteristic(PRESETS_UUID);
        console.log("Preset characteristic connected");
        await readPresets();
    } catch {
        firmwareWarning();
    }
}
async function readPresets() {
    var value = await presetRef.readValue();
    value = decoder.decode(value);
    console.log("Presets: " + value);
    presetListElement.innerHTML = "";
    for(singleValue of value.split(",")){
        var option = document.createElement("option");
        option.text = singleValue;
        option.value = singleValue;
        presetListElement.add(option)
    }
    presetListElement.ariaInvalid = false;
    setTimeout(function() {clearCheck(presetListElement);},2000);
}
async function writePresets(value) {
    console.log("Write Preset: " + value);
    value = encoder.encode(value);
    await presetRef.writeValue(value);
}
async function runPreset() {
    var value = presetListElement.value;
    writePresets(":"+value);
}
async function savePreset() {
    var value = document.getElementById("presetName").value;
    await writePresets(">" + value);
    await readPresets();
    presetListElement.value = value;
}
async function deletePreset() {
    var value = presetListElement.value;
    await writePresets("<" + value);
    await readPresets();
}
async function factoryReset() {
    await writePresets("^");
    await readPresets();
}

var statusRef, controlRef, speedElement, maxDepthElement, minDepthElement;
async function initStatus() {
    speedElement = document.getElementById("speed");
    maxDepthElement = document.getElementById("maxDepth");
    minDepthElement = document.getElementById("minDepth");
    try{
        statusRef = await serviceRef.getCharacteristic(STATUS_UUID);
        await statusRef.startNotifications().then(function() {
            statusRef.addEventListener('characteristicvaluechanged', handleStatus);
        });
        controlRef = await serviceRef.getCharacteristic(CONTROL_UUID);
        console.log("Status and control characteristics connected");
        readStatus();
    } catch {
        firmwareWarning();
    }
}
function setStatus(value) {
    value = decoder.decode(value);
    console.log("Status: " + value);
    speedElement.value = value.split(",")[6].split(":")[0];
    maxDepthElement.value = value.split(",")[0].split(":")[0];
    minDepthElement.value = value.split(",")[1].split(":")[0];
}
async function readStatus() {
    var value = await statusRef.readValue();
    setStatus(value);
}
function handleStatus(event) {
    var value = event.target.value;
    setStatus(value);
}
async function writeControl(value) {
    console.log("Write Control: " + value);
    value = encoder.encode(value);
    await controlRef.writeValue(value);
}
async function setSpeed() {
    writeControl("6:" + speedElement.value + ",");
}
async function setMaxDepth() {
    var max = parseInt(maxDepthElement.value);
    var min = parseInt(minDepthElement.value);
    if (max < min) {
        maxDepthElement.value = min + 1;
    }
    writeControl("0:" + maxDepthElement.value + ",");
}
async function setMinDepth() {
    var max = parseInt(maxDepthElement.value);
    var min = parseInt(minDepthElement.value);
    if (min > max) {
        minDepthElement.value = max - 1;
    }
    writeControl("1:" + minDepthElement.value + ",");
}


async function writeUpdate() {
    var update = await serviceRef.getCharacteristic(UPDATE_UUID);
    await update.writeValue(encoder.encode(true));
}

async function connectMotorPage() {
    await handleConnect();
    await initMaxAcceleration();
    await initMotorRPM();
    await initStepsPerRevolution();
    await initPulleyTeeth();
    await initBeltPitch();
}

async function connectHomingPage() {
    await handleConnect();
    await initHomingType();
    await initRailLength();
    await initHomeBetween();
}

async function connectRailPage() {
    await handleConnect();
    await initReverseRail();
}

async function connectNamePage() {
    await handleConnect();
    await initDeviceName();
}

async function connectWiFiPage() {
    await handleConnect();
    await initWiFi();
}

async function connectPresets() {
    await handleConnect();
    await initPresets();
    await initStatus();
}
