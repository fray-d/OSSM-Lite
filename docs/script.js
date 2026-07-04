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

//Common Settings
const SPEED_UUID = "4f53534d-436f-6d6d-6f6e-005370656564";
const MAXDEP_UUID = "4f53534d-436f-6d6d-6f6e-4d6178446570";
const MINDEP_UUID = "4f53534d-436f-6d6d-6f6e-4d696e446570";
const BUFFER_UUID = "4f53534d-436f-6d6d-6f6e-427566666572";
const OFFSET_UUID = "4f53534d-436f-6d6d-6f6e-4f6666736574";
const STREAM_UUID = "4f53534d-436f-6d6d-6f6e-53747265616d";

const encoder = new TextEncoder();
const decoder = new TextDecoder();

function decodeHex(value) {
    value = value.replaceAll("-","");
    value = Uint8Array.fromHex(value);
    value = new TextDecoder().decode(value);
    return value;
}


function clearCheck(element) {
    element.removeAttribute('aria-invalid');
}

// Main server/service
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
        firmwareWarning();
    }
    connectButton.ariaBusy = false;
    connectButton.ariaDisabled = false;
}

function handleDisconnect(){
    document.getElementById("connect").style.display = 'block';
    document.getElementById("controls").style.display = "none";
}

function firmwareWarning(){
    try {
        serverRef.device.gatt.disconnect();
        document.getElementById("oldFirmware").open = true;
    } catch {
        console.log("Nothing to disconnect");
    }
}
function firmareClose() {
    document.getElementById("oldFirmware").open = false;
}

// Device Name
async function initDeviceName() {
    let deviceNameElement = document.getElementById("deviceName");
    let saveButton = document.getElementById("save");
    try{
        let deviceNameRef = await serviceRef.getCharacteristic(DEVICE_NAME_UUID);

        console.log("Characteristic " + decodeHex(DEVICE_NAME_UUID) + " connected.")
        saveButton.onclick = function() {
            writeSetting(deviceNameElement, deviceNameRef);
        }
        readSetting(null, deviceNameElement, deviceNameRef);
    } catch {
        firmwareWarning();
    }
}

// Wifi
async function initWiFi() {
    try{
        let wifiRef = await serviceRef.getCharacteristic(WIFI_UUID);
        console.log("Characteristic " + decodeHex(WIFI_UUID) + " connected.")
        document.getElementById("save").onclick = function() {
            writeWiFi(wifiRef);
        }
        document.getElementById("update").onclick = function() {
            writeUpdate();
        }
        readWiFi(wifiRef);
    } catch {
        firmwareWarning();
    }
}
async function readWiFi(wifiRef) {
    document.getElementById("update").style.display = 'none';
    let ssidElement = document.getElementById("SSID");
    let passwordElement = document.getElementById("Password");
    var value = await wifiRef.readValue();
    value = decoder.decode(value);
    value = JSON.parse(value)
    ssidElement.value = value['ssid'];
    ssidElement.ariaInvalid = !value['connected'];
    if (value['connected']) {
        document.getElementById("update").style.display = 'block';
    }
    passwordElement.value = "";
}
async function writeWiFi() {
    var ssid = document.getElementById("SSID").value;
    var password = document.getElementById("Password").value;
    var value = "set:wifi:" + ssid + "|" + password;
    console.log("Write WiFi: " + ssid);
    value = encoder.encode(value);
    await wifiRef.writeValue(value);
    readWiFi();
}

async function writeUpdate() {
    var update = await serviceRef.getCharacteristic(UPDATE_UUID);
    await update.writeValue(encoder.encode(true));
}

// Presets
var presetRef, presetListElement;
async function initPresets() {
    presetListElement = document.getElementById("presetList");
    try{
        presetRef = await serviceRef.getCharacteristic(PRESETS_UUID);
        console.log("Characteristic " + decodeHex(PRESETS_UUID) + " connected");
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
        console.log("Characteristic " + decodeHex(STATUS_UUID) + " connected.")
        await statusRef.startNotifications().then(function() {
            statusRef.addEventListener('characteristicvaluechanged', handleStatus);
        });
        controlRef = await serviceRef.getCharacteristic(CONTROL_UUID);
        console.log("Characteristic " + decodeHex(CONTROL_UUID) + " connected.")
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
async function setASpeed() {
    writeControl("6:" + speedElement.value + ",");
}
async function setAMaxDepth() {
    var max = parseInt(maxDepthElement.value);
    var min = parseInt(minDepthElement.value);
    if (max < min) {
        maxDepthElement.value = min + 1;
    }
    writeControl("0:" + maxDepthElement.value + ",");
}
async function setAMinDepth() {
    var max = parseInt(maxDepthElement.value);
    var min = parseInt(minDepthElement.value);
    if (min > max) {
        minDepthElement.value = max - 1;
    }
    writeControl("1:" + minDepthElement.value + ",");
}

// Shared Settings
async function initSetting(element, uuid) {
    try {
        let characteristicRef = await serviceRef.getCharacteristic(uuid);
        console.log("Characteristic " + decodeHex(uuid) + " connected.")
        if (characteristicRef.properties.notification) {
            await characteristicRef.startNotifications().then(
                function() {
                    characteristicRef.addEventListener('characteristicvaluechanged', (event) => readSetting(event, element, characteristicRef));
                }
            )
        }
        element.onchange = function() {
            writeSetting(element, characteristicRef);
        };
        await readSetting(null, element, characteristicRef);
    } catch {
       firmwareWarning();
    }
}

async function readSetting(event, element, characteristicRef) {
    var value;
    if (event != null) {
        value = event.target.value;
    } else {
        value = await characteristicRef.readValue();
    }
    value = decoder.decode(value);
    console.log("Received: " + value);
    switch (element.type) {
        case "checkbox":
            element.checked = value.toLocaleLowerCase() == "true";
            break;
        default:
            element.value = value;
            break;
    }
    element.focus();
    element.ariaInvalid = false;
    setTimeout(function() {clearCheck(element);}, 2000);
}

async function writeSetting(element,characteristicRef) {
    var value;
    switch (element.type) {
        case "checkbox":
            value = element.checked;
            break;
        default:
            value = element.value;
            break;
    }
    console.log("Sent: " + value);
    value = encoder.encode(value);
    await characteristicRef.writeValueWithoutResponse(value);

    const videoPlayer = document.getElementById("video");
    if (videoPlayer != null) {
        if (videoPlayer.paused || videoPlayer.currentTime == 0) {
            switch (element.id) {
                case "minDepth":
                    sendStream(100,2000);
                    break;
                case "maxDepth":
                    sendStream(0,2000);
                    break;
                default:
                    break;
            }
        }
    }

    if (!characteristicRef.properties.notification) {
        await readSetting(null, element, characteristicRef);
    }
}

let streamRef;
async function initStream() {
    try {
        streamRef = await serviceRef.getCharacteristic(STREAM_UUID);
        console.log("Characteristic " + decodeHex(STREAM_UUID) + " connected.")
        await streamRef.readValue();
    } catch {
        firmwareWarning();
    }
}

async function loadVideo(element) {
    const file = element.files[0];
    if (file) {
        const fileURL = URL.createObjectURL(file); 
        document.getElementById("video").src = fileURL;
    }
}

async function loadFunscript(element) {
    const file = element.files[0];
    if (file) {
        const reader = new FileReader();
        reader.onload = (event) => {
            if (parseFunscript(event.target.result)) {
           //     resetPlayback();
            }
        };
        reader.readAsText(file);
    }
}

var funscriptData = {};
var currentAction = 0;
async function parseFunscript(content) {
    funscriptData = {};
    var csv = content.split("\n");
    if (csv.length > 0) {
        funscriptData.actions = csv.map(item => ({
            pos: Number(item.split(",")[1]),
            at: Number(item.split(",")[0])
        }));
    }
    if (!funscriptData.actions || !Array.isArray(funscriptData.actions)) {
        console.log("Could not parse funscript");
        return false;
    }
    var lastDirection = 0;
    funscriptData.simpleActions = [];
    funscriptData.actions.forEach(
      function(value,index){
        var nextValue = funscriptData.actions[index+1];
        if (nextValue){
          var direction = (value.pos-nextValue.pos)/Math.abs(value.pos-nextValue.pos);
          if (direction != lastDirection){
            funscriptData.simpleActions.push(funscriptData.actions[index]);
          }
          lastDirection = direction;
        }
      }
    )
    return true;
}

async function sendStream(pos, dur) {
    let value = pos + ":" + dur;
    console.log("Stream: " + value);
    value = encoder.encode(value);
    streamRef.writeValueWithoutResponse(value);

    let dist = pos - document.getElementById("progress").value;
    let count = dur/100;
    step = dist/count;
    const id = setInterval(() => {
        count--;
        document.getElementById("progress").value += step;
        if (count < 0) {
            clearInterval(id);
        }
    }, 100);
}

async function syncFunscript() {
    var actions = funscriptData.actions;
    // if (isSimple) {
    //     actions = funscriptData.simpleActions;
    // }
    const currentTime = document.getElementById("video").currentTime * 1000;
    const action = actions[currentAction];
    if (action.at > currentTime) {
        return;
    }
    if (currentAction < actions.length) {
        const nextAction = actions[currentAction + 1];
        let duration = nextAction.at - action.at;
        sendStream(nextAction.pos, duration);
    }
    currentAction++;
}

let intervalTracker;
async function playStream() {
    console.log("play");
    intervalTracker = setInterval(syncFunscript, 2);
}

async function pauseStream() {
    console.log("pause");
    clearInterval(intervalTracker);
}

async function seekStream() {
    const currentTime = document.getElementById("video").currentTime * 1000;
    var actions = funscriptData.actions;
    currentAction = actions.findIndex(a => a.at > currentTime);
    if (currentAction === -1) {
        currentAction = actions.length;
    }
    console.log("Current action: " + currentAction);
}

async function connectMotorPage() {
    await handleConnect();
    await initSetting(document.getElementById("maxAcceleration"), MACCEL_UUID);
    await initSetting(document.getElementById("motorRPM"), MAXRPM_UUID);
    await initSetting(document.getElementById("stepsPerRevolution"), STEPPR_UUID);
    await initSetting(document.getElementById("pulleyTeeth"), PULLEY_UUID);
    await initSetting(document.getElementById("beltPitch"), BPITCH_UUID);
}

async function connectHomingPage() {
    await handleConnect();
    await initSetting(document.getElementById("homingType"), HOMING_TYPE_UUID);
    await initSetting(document.getElementById("railLength"), RAIL_LENGTH_UUID);
    await initSetting(document.getElementById("homeBetweenModes"), HOME_BETWEEN_MODES_UUID);
}

async function connectRailPage() {
    await handleConnect();
    await initSetting(document.getElementById("reverseRail"), REVERSE_RAIL_UUID);
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

async function connectFunscript() {
    await handleConnect();
    await initStream();
    await initSetting(document.getElementById('speed'),SPEED_UUID);
    await initSetting(document.getElementById('maxDepth'), MAXDEP_UUID);
    await initSetting(document.getElementById('minDepth'), MINDEP_UUID);
}