const LEGACY_OSSM_UUID = "522b443a-4f53-534d-0001-420badbabe69";
const OSSM_MOTION_UUID = "0000055e-0000-1000-8000-00805f9b34fb";
const OSSM_CONFIG_UUID = "0000055c-0000-1000-8000-00805f9b34fb";
const OSSM_ADVANCED_UUID = "0000055a-0000-1000-8000-00805f9b34fb";
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
const SENSOR_UUID = "4f53534d-436f-6e66-6967-53656e736f72";
const SPDCRV_UUID = "4f53534d-436f-6e66-6967-537064437276";
const SPDHOM_UUID = "4f53534d-436f-6e66-6967-537064486f6d";

//AP Mode
const PRESETS_UUID = "4f53534d-6164-7661-6e63-656470727374";
const STATUS_UUID = "4f53534d-6164-7661-6e63-656473746174";
const CONTROL_UUID = "4f53534d-6164-7661-6e63-6564636f6e74";

//Common Settings
const SPEED_UUID = "4f53534d-436f-6d6d-6f6e-005370656564";
const MAXDEP_UUID = "4f53534d-436f-6d6d-6f6e-4d6178446570";
const MINDEP_UUID = "4f53534d-436f-6d6d-6f6e-4d696e446570";
const BUFFER_UUID = "4f53534d-436f-6d6d-6f6e-427566666572";
const OFFSET_UUID = "4f53534d-436f-6d6d-6f6e-4f6666736574";
const STREAM_UUID = "4f53534d-436f-6d6d-6f6e-53747265616d";

//Stroke Engine
const SENSAT_UUID = "4f53534d-456e-6769-6e65-536174696f6e";
const SENPAT_UUID = "4f53534d-456e-6769-6e65-50617465726e";
const SENPTL_UUID = "4f53534d-456e-6769-6e65-5061744c7374";

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
var serverRef, motionRef, configRef, advancedRef
async function handleConnect() {
    var connectButton = document.getElementById("connect");
    connectButton.ariaBusy = true;
    connectButton.ariaDisabled = true;
    try {
        const bleDevice = await navigator.bluetooth.requestDevice({
            filters:[
                {services:[OSSM_CONFIG_UUID]},
                {services:[OSSM_MOTION_UUID]},
                {services:[OSSM_ADVANCED_UUID]}
            ]
        });
        bleDevice.addEventListener('gattserverdisconnected', handleDisconnect);
        console.log("Device Found");

        serverRef = await bleDevice.gatt.connect();
        console.log("Connected to server");
        
        motionRef = await serverRef.getPrimaryService(OSSM_MOTION_UUID);
        configRef = await serverRef.getPrimaryService(OSSM_CONFIG_UUID);
        advancedRef = await serverRef.getPrimaryService(OSSM_ADVANCED_UUID);
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
        let deviceNameRef = await configRef.getCharacteristic(DEVICE_NAME_UUID);

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
        let wifiRef = await configRef.getCharacteristic(WIFI_UUID);
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
async function writeWiFi(wifiRef) {
    var ssid = document.getElementById("SSID").value;
    var password = document.getElementById("Password").value;
    var value = "set:wifi:" + ssid + "|" + password;
    console.log("Write WiFi: " + ssid);
    value = encoder.encode(value);
    await wifiRef.writeValue(value);
    await readWiFi(wifiRef);
}

async function writeUpdate() {
    var update = await configRef.getCharacteristic(UPDATE_UUID);
    await update.writeValue(encoder.encode(true));
}

// Presets
var presetRef, presetListElement;
async function initPresets() {
    presetListElement = document.getElementById("presetList");
    try{
        presetRef = await advancedRef.getCharacteristic(PRESETS_UUID);
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
    await writePresets(":"+value);
    readStatus();
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

var statusRef, controlRef, speedElement, maxDepthElement, minDepthElement, inSpeedElement, outSpeedElement, inAccelElement, outAccelElement;
var elements = ['maxDepth','minDepth','inSpeed','outSpeed','inAccel','outAccel','speed'];
var modifiers = ['','Amplitude','ToMax','AtMax','ToMin','AtMin','Offset'];
async function initStatus() {
    maxDepthElement = document.getElementById("maxDepth");
    minDepthElement = document.getElementById("minDepth");
    inSpeedElement = document.getElementById("inSpeed");
    outSpeedElement = document.getElementById("outSpeed");
    inAccelElement = document.getElementById("inAccel");
    outAccelElement = document.getElementById("outAccel");
    speedElement = document.getElementById("speed");
    try{
        statusRef = await advancedRef.getCharacteristic(STATUS_UUID);
        console.log("Characteristic " + decodeHex(STATUS_UUID) + " connected.")
        await statusRef.startNotifications().then(function() {
            statusRef.addEventListener('characteristicvaluechanged', handleStatus);
        });
        controlRef = await advancedRef.getCharacteristic(CONTROL_UUID);
        console.log("Characteristic " + decodeHex(CONTROL_UUID) + " connected.")
        readStatus();
    } catch {
        firmwareWarning();
    }
}
function setStatus(value) {
    value = decoder.decode(value);
    console.log("Status: " + value);
    elements.forEach((base, i) => {
        modifiers.forEach((mod, j) => {
            var element = document.getElementById(base + mod);
            if (element != null) {
                var sub = value.split(",")[i].split(":");
                if (sub != null && sub.length > j) {
                    element.value = sub[j];
                } else {
                    element.value = element.placeholder;
                }
            }
        });
    });
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
async function setAInSpeed() {
    writeControl("2:" + inSpeedElement.value + ",")
}
async function setAOutSpeed() {
    writeControl("3:" + outSpeedElement.value + ",")
}
async function setAInAccel() {
    writeControl("4:" + inAccelElement.value + ",")
}
async function setAOutAccel() {
    writeControl("5:" + outAccelElement.value + ",")
}
async function setAAmplitude(element) {
    elements.forEach((base, i) => {
        if (element.id.includes(base)) {
            writeControl(i+":0:"+element.value + ",");
        }
    });
}
async function setAToMax(element) {
    elements.forEach((base, i) => {
        if (element.id.includes(base)) {
            writeControl(i+":1:"+element.value + ",");
        }
    });
}
async function setAAtMax(element) {
    elements.forEach((base, i) => {
        if (element.id.includes(base)) {
            writeControl(i+":2:"+element.value + ",");
        }
    });
}
async function setAToMin(element) {
    elements.forEach((base, i) => {
        if (element.id.includes(base)) {
            writeControl(i+":3:"+element.value + ",");
        }
    });
}
async function setAAtMin(element) {
    elements.forEach((base, i) => {
        if (element.id.includes(base)) {
            writeControl(i+":4:"+element.value + ",");
        }
    });
}
async function setAOffset(element) {
    elements.forEach((base, i) => {
        if (element.id.includes(base)) {
            writeControl(i+":5:"+element.value + ",");
        }
    });
}
async function getPresetString() {
    var output = "";
    elements.slice(0, -1).forEach((base, i) => {
        output += document.getElementById(base).value;
        modifiers.slice(1).forEach((mod, j) => {
            var element = document.getElementById(base + mod);
            if (element != null) {
                if (element.value != "") {
                    output += ":" + element.value;
                } else {
                    output += ":" + element.placeholder;
                }
            }
        });
        output += ",";
    });
    document.getElementById("presetString").value = output;
}
async function setPresetString() {
    var value = document.getElementById("presetString").value;
    var c = 1;
    var element = document.getElementById("speed");
    element.value = 0;
    element.dispatchEvent(new Event('change'));
    elements.slice(0,-1).forEach((base, i) => {
        var sub = value.split(",")[i].split(":");
        modifiers.forEach((mod, j) => {
            element = document.getElementById(base + mod);
            if (element != null) {
                if (sub != null && sub.length > j) {
                    if ((element.value == "" && element.placeholder != sub[j]) || (element.value != "" && element.value != sub[j])) {
                        setTimeout(() => {
                            element = document.getElementById(base + mod);
                            element.value = sub[j];
                            element.dispatchEvent(new Event('change'));
                        }, c * 250);
                        c++;
                    }
                }
            }
        });
    });
}

function drawChart() {
const canvas = document.getElementById("speedCurveChart");
    const ctx = canvas.getContext("2d");
    const exp = document.getElementById("speedCurve").value;
    ctx.reset();
    chartOffset = 25;
    chartWidth = canvas.width - chartOffset;
    chartHeight = canvas.height -  chartOffset;
    ctx.strokeStyle = 'rgb(228, 133, 0)';
    ctx.fillStyle = ctx.strokeStyle;
    ctx.font = "20px Arial";
    ctx.textAlign = "center";

    ctx.beginPath();
    ctx.moveTo(chartOffset, 0);
    ctx.lineTo(chartOffset, chartHeight);
    ctx.lineTo(canvas.width, chartHeight);
    ctx.moveTo(chartOffset, chartHeight);

    for (let step = 0; step < chartWidth; step++) {
        const rat = step/chartWidth;
        const value = Math.pow(1-Math.pow(1-rat, exp), 1 / exp);         
        const h = chartHeight - (value * chartHeight);
        ctx.lineTo(step+chartOffset,h);
    }
    ctx.stroke();
    ctx.fillText("Speed Effect", chartWidth/2 + 20, canvas.height - 5);
    ctx.translate(20,  chartHeight / 2 - 20);
    ctx.rotate(-Math.PI/2);
    ctx.fillText("Speed Knob", 0,0);
}

// Shared Settings
async function initCurve() {
    const element = document.getElementById("speedCurve");
    try {
        let characteristicRef = await configRef.getCharacteristic(SPDCRV_UUID);
        console.log("Characteristic " + decodeHex(SPDCRV_UUID) + " connected.")
        element.onchange = function() {
            writeSetting(element, characteristicRef);
            drawChart();
        };
        await characteristicRef.startNotifications().then(
            function() {
                characteristicRef.addEventListener('characteristicvaluechanged', (event) => readSetting(event, element, characteristicRef));
            }
        )
        await readSetting(null, element, characteristicRef);
        drawChart();
    } catch {
       firmwareWarning();
    }
}

// Shared Settings
async function initSetting(element, uuid, service) {
    let characteristicRef;
    try {
        characteristicRef = await service.getCharacteristic(uuid);
        console.log("Characteristic " + decodeHex(uuid) + " connected.")

        if (characteristicRef.properties.notify) {
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
       return;
    }

    try {
        const c2901 = await characteristicRef.getDescriptor(0x2901);
        const raw = await c2901.readValue();
        const decoder8 = new TextDecoder('utf-8');
        const name = decoder8.decode(raw);
        const c2904 = await characteristicRef.getDescriptor(0x2904);
        const value = await c2904.readValue();
        const format = value.getUint8(0);
        const ordinal = value.getUint16(5,true);
        console.log(`Name: ${name}, Format: ${format}, Ordinal: ${ordinal}`);
        const c2906 = await characteristicRef.getDescriptor(0x2906);
        const range = await c2906.readValue();
        switch (format) {
            case 23:
                element.min = range.getFloat32(0, true);
                element.max = range.getFloat32(4, true);
                break;
            case 6:
                element.min = range.getUint16(0, true);
                element.max = range.getUint16(2, true);
                break;
            default:
                break; 
        }
        console.log(`Min: ${element.min}, Max: ${element.max}`);
    } catch {
        console.log("Could not find descriptors for min/max/format/ordinal");
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
    await characteristicRef.writeValue(value);

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

    if (!characteristicRef.properties.notify) {
        await new Promise(resolve => setTimeout(resolve, 250));
        await readSetting(null, element, characteristicRef);
    }
}

let streamRef;
async function initStream() {
    try {
        streamRef = await motionRef.getCharacteristic(STREAM_UUID);
        console.log("Characteristic " + decodeHex(STREAM_UUID) + " connected.")
        await sendStream(0,1000);
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
    try {
      funscriptData = JSON.parse(content);
    } catch (err) {
        console.log("failed to parse as json: " + err + " Attempting CSV");
        var csv = content.split("\n");
        if (csv.length > 0) {
            funscriptData.actions = csv.map(item => ({
                pos: Number(item.split(",")[1]),
                at: Number(item.split(",")[0])
            }));
        }
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
    if (document.getElementById("reverse").checked) {
        pos = 100 - pos;
    }
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
    let actions = funscriptData.actions;
    if (document.getElementById("simplify").checked) {
        actions = funscriptData.simpleActions;
    }
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
    if (document.getElementById("simplify").checked) {
        actions = funscriptData.simpleActions;
    }
    currentAction = actions.findIndex(a => a.at > currentTime);
    if (currentAction === -1) {
        currentAction = actions.length;
    }
    console.log("Current action: " + currentAction);
}

// Stroke Engine Patterns
var patternListRef, patternListElement;
async function initStrokeEngine() {
    patternListElement = document.getElementById("patternList");
    try{
        patternListRef = await motionRef.getCharacteristic(SENPTL_UUID);
        console.log("Characteristic " + decodeHex(SENPTL_UUID) + " connected");
    } catch {
        firmwareWarning();
    }
    await readPatterns();
}
async function readPatterns() {
    var c = 0;
    patternListElement.innerHTML = "";
    while (c >= 0) {
        await patternListRef.writeValue(encoder.encode(c));
        var value = await patternListRef.readValue();
        value = decoder.decode(value);
        if (value != "") {
            var option = document.createElement("option");
            option.text = value.split(":")[0];
            option.value = c;
            option.title = value.split(":")[1];
            patternListElement.add(option);
            c++;
        } else {
            c = -1;
        }
    }
}
async function syncPatternDescription(element) {
    document.getElementById("patternDescription").innerText = element.selectedOptions[0].title;
}
async function initPatternSetting(element, uuid) {
    let characteristicRef;
    try {
        characteristicRef = await motionRef.getCharacteristic(uuid);
        console.log("Characteristic " + decodeHex(uuid) + " connected.")
        if (characteristicRef.properties.notify) {
            await characteristicRef.startNotifications().then(
                function() {
                    characteristicRef.addEventListener('characteristicvaluechanged', (event) => {
                        readSetting(event, element, characteristicRef);
                        syncPatternDescription(element);
                    });
                }
            )
        }
        element.onchange = function() {
            writeSetting(element, characteristicRef);
            syncPatternDescription(element);
        };
        await readSetting(null, element, characteristicRef);
        syncPatternDescription(element);
        writeSetting(element, characteristicRef);
    } catch {
       firmwareWarning();
       return;
    }

    try {
        const c2901 = await characteristicRef.getDescriptor(0x2901);
        const raw = await c2901.readValue();
        const decoder8 = new TextDecoder('utf-8');
        const name = decoder8.decode(raw);
        const c2904 = await characteristicRef.getDescriptor(0x2904);
        const value = await c2904.readValue();
        const format = value.getUint8(0);
        const ordinal = value.getUint16(5,true);
        console.log(`Name: ${name}, Format: ${format}, Ordinal: ${ordinal}`);
        const c2906 = await characteristicRef.getDescriptor(0x2906);
        const range = await c2906.readValue();
        switch (format) {
            case 23:
                element.min = range.getFloat32(0, true);
                element.max = range.getFloat32(4, true);
                break;
            case 6:
                element.min = range.getUint16(0, true);
                element.max = range.getUint16(2, true);
                break;
            default:
                break; 
        }
        console.log(`Min: ${element.min}, Max: ${element.max}`);
    } catch {
        console.log("Could not find descriptors for min/max/format/ordinal");
    }
}

async function connectMotorPage() {
    await handleConnect();
    await initSetting(document.getElementById("maxAcceleration"), MACCEL_UUID, configRef);
    await initSetting(document.getElementById("motorRPM"), MAXRPM_UUID, configRef);
    await initSetting(document.getElementById("stepsPerRevolution"), STEPPR_UUID, configRef);
    await initSetting(document.getElementById("pulleyTeeth"), PULLEY_UUID, configRef);
    await initSetting(document.getElementById("beltPitch"), BPITCH_UUID, configRef);
}

async function connectHomingPage() {
    await handleConnect();
    await initSetting(document.getElementById("homingType"), HOMING_TYPE_UUID, configRef);
    await initSetting(document.getElementById("railLength"), RAIL_LENGTH_UUID, configRef);
    await initSetting(document.getElementById("currentLimit"), SENSOR_UUID, configRef);
    await initSetting(document.getElementById("homingSpeed"), SPDHOM_UUID, configRef);
    await initSetting(document.getElementById("homeBetweenModes"), HOME_BETWEEN_MODES_UUID, configRef);
}

async function connectRailPage() {
    await handleConnect();
    await initSetting(document.getElementById("reverseRail"), REVERSE_RAIL_UUID, configRef);
}

async function connectNamePage() {
    await handleConnect();
    await initDeviceName();
}

async function connectCurvePage() {
    await handleConnect();
    await initCurve();
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
    await initSetting(document.getElementById('speed'),SPEED_UUID, motionRef);
    await initSetting(document.getElementById('maxDepth'), MAXDEP_UUID, motionRef);
    await initSetting(document.getElementById('minDepth'), MINDEP_UUID, motionRef);
    document.getElementById("simplify").onchange = function() {
        seekStream();
    };
}

async function connectStrokeEngine() {
    await handleConnect();
    await initStrokeEngine();
    await initPatternSetting(patternListElement, SENPAT_UUID);
    await initSetting(document.getElementById('speed'),SPEED_UUID, motionRef);
    await initSetting(document.getElementById('maxDepth'), MAXDEP_UUID, motionRef);
    await initSetting(document.getElementById('minDepth'), MINDEP_UUID, motionRef);
    await initSetting(document.getElementById('sensation'), SENSAT_UUID, motionRef);
    
}