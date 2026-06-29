const OSSM_UUID = "522b443a-4f53-534d-0001-420badbabe69";
const HOMING_TYPE_UUID = "522b443a-4f53-534d-1060-420badbabe69";
const RAIL_LENGTH_UUID = "522b443a-4f53-534d-1070-420badbabe69";
const HOME_BETWEEN_MODES_UUID = "522b443a-4f53-534d-1080-420badbabe69";
const REVERSE_RAIL_UUID = "522b443a-4f53-534d-1050-420badbabe69";
const DEVICE_NAME_UUID = "522b443a-4f53-534d-1040-420badbabe69";

var serverRef, serviceRef, homeBetweenRef

const encoder = new TextEncoder();
const decoder = new TextDecoder();

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

var homingTypeRef, homingElement;
async function initHomingType() {
    homingElement = document.getElementById("homingType");
    homingTypeRef = await serviceRef.getCharacteristic(HOMING_TYPE_UUID);
    console.log("Homing characteristic connected");
    readHomingType();
}
async function readHomingType() {
    var value = await homingTypeRef.readValue();
    value = decoder.decode(value);
    console.log("Homing Type: " + value);
    homingElement.value = value;
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
    railLengthRef = await serviceRef.getCharacteristic(RAIL_LENGTH_UUID);
    console.log("Rail Length characteristic connected");
    readRailLength();
}
async function readRailLength() {
    var value = await railLengthRef.readValue();
    value = decoder.decode(value);
    console.log("Rail Length: " + value);
    railLengthElement.value = value;
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
    homeBetweenRef = await serviceRef.getCharacteristic(HOME_BETWEEN_MODES_UUID);
    console.log("Home between modes characteristic connected");
    readHomeBetween();
}
async function readHomeBetween() {
    var value = await homeBetweenRef.readValue();
    value = decoder.decode(value);
    console.log("Homing Between Modes: " + value);
    homeBetweenElement.checked = value.toLowerCase() === "true";
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
    reverseRailRef = await serviceRef.getCharacteristic(REVERSE_RAIL_UUID);
    console.log("Reverse rail characteristic connected");
    readReverseRail();
}
async function readReverseRail() {
    var value = await reverseRailRef.readValue();
    value = decoder.decode(value);
    console.log("Reverse rail: " + value);
    reverseRailElement.checked = value.toLowerCase() === "true";
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
    deviceNameRef = await serviceRef.getCharacteristic(DEVICE_NAME_UUID);
    console.log("Device name characteristic connected");
    readDeviceName();
}
async function readDeviceName() {
    var value = await deviceNameRef.readValue();
    value = decoder.decode(value);
    console.log("Device Name: " + value);
    deviceNameElement.value = value;
}
async function writeDeviceName() {
    var value = deviceNameElement.value;
    console.log("Write Device Name: " + value);
    value = encoder.encode(value);
    deviceNameRef.writeValueWithoutResponse(value);
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