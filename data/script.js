let lock = false;

let messageAnalog = new Uint16Array(5); // 10 Byte
let messageDigital = new Uint8Array(6); //  6 Byte
let lastMessageTimestamp = 0;
let websocketOpen = false;

let focusSlider = undefined;
let focusLabel = undefined;
let irisSlider = undefined;
let irisLabel = undefined;
let zoomSlider = undefined;
let zoomLabel = undefined;

function updateSlider(element, final = false) {
  if (!lock || final) {
    lock = true;
    var value = element.value;
    if (element.id == "focus") {
      messageAnalog[0] = value; 
      focusLabel.innerHTML = value;
    }
    else if (element.id == "iris") {
      messageAnalog[1] = value;
      irisLabel.innerHTML = value;
    }
    else console.log("no place in message array");

    if (websocketOpen) {
      sendToWebsocket();
    } else {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function () {
        if (xhr.readyState == XMLHttpRequest.DONE) {
          lock = false;
        }
      };
      xhr.open("GET", "/" + element.id + "?value=" + value, true);
      xhr.send();
    }
    lock = false;
  }
}

function updateSliderZoom(element, final = false) {
  if (!lock || final) {
    lock = true;
    if (final === true) {
      element.value = 500;
    }
    var zoomValue = element.value;
    if (element.id == "zoom") {
      messageAnalog[2] = zoomValue;
      zoomLabel.innerHTML = zoomValue;
    }
    else console.log("no place in message array");

    if (websocketOpen) {
      sendToWebsocket();
    } else {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function () {
        if (xhr.readyState == XMLHttpRequest.DONE) {
          lock = false;
        }
      };
      xhr.open("GET", "/" + element.id + "?value=" + zoomValue, true);
      xhr.send();
    }
    lock = false;
  }
}

/*****************
 * PING function
 */
var intervalId = setInterval(Pinger_ping, 3000);

function Pinger_ping() {
  if (lastMessageTimestamp + 1000 < Date.now()) {
    ping = new XMLHttpRequest();
    ping.timeout = 1000;
    ping.onreadystatechange = function () {
      if (ping.readyState == 4) {
        if (ping.status == 200) {
          document.body.style.backgroundColor = "white";
        } else {
          document.body.style.backgroundColor = "red";
        }
      }
    };
    ping.ontimeout = function (e) {
      document.body.style.backgroundColor = "red";
    };
    ping.open("GET", "/ping", true);
    ping.send();
  }
}


/*******************************************
 * lock inputs to avoid overkill on server
 */
var intervalId = setInterval(lockInput, 1000);

function lockInput() {
  if (lastMessageTimestamp + 1000 <= Date.now()) {
    focusSlider.removeAttribute('disabled');
    irisSlider.removeAttribute('disabled');
    zoomSlider.removeAttribute('disabled');
  }
  else {
    focusSlider.setAttribute('disabled','disabled');
    irisSlider.setAttribute('disabled','disabled');
    zoomSlider.setAttribute('disabled','disabled');
  }
}

/***********************
 * WebSocket functions
 */

// #region WebSocket functions
var gateway = `ws://${window.location.hostname}/ws`;
var ws;
function initWebSocket() {
  console.log("Trying to open a WebSocket connection...");
  ws = new WebSocket(gateway);
  ws.binaryType = "arraybuffer";
  ws.onopen = onOpen;
  ws.onclose = onClose;
  ws.onmessage = onMessage;
}

function onOpen(event) {
  console.log("Connection opened");
  websocketOpen = true;
}

function onClose(event) {
  console.log("Connection closed");
  websocketOpen = false;
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  var data = event.data;
  var dv = new DataView(data);
  var focus = dv.getUint16(0, true);
  var iris = dv.getUint16(2, true);
  var zoom = dv.getUint16(4, true);
  var focus_ll = dv.getUint16(6, true);
  var focus_ul = dv.getUint16(8, true);

  var autoIris = dv.getUint8(10, true) === 1;
  var record = dv.getUint8(11, true) === 1;
  var prog_mode = dv.getUint8(12, true) === 1;
  var preset1 = dv.getUint8(13, true) === 1;
  var preset2 = dv.getUint8(14, true) === 1;
  var preset3 = dv.getUint8(15, true) === 1;

  messageAnalog[0] = focus;
  messageAnalog[1] = iris;
  messageAnalog[2] = zoom;
  messageAnalog[3] = focus_ll;
  messageAnalog[4] = focus_ul;

  console.log("Focus: " + focus);

  setFocus(focus);
  setIris(iris);
  setZoom(zoom);
  lastMessageTimestamp = Date.now();
}

function sendToWebsocket() {
  var message = new Uint8Array(
    messageAnalog.byteLength + messageDigital.byteLength
  );
  message.set(
    new Uint8Array(
      messageAnalog.buffer,
      messageAnalog.byteOffset,
      messageAnalog.byteLength
    ),
    0
  );
  message.set(messageDigital, messageAnalog.byteLength);
  ws.send(message);
}
// #endregion

window.addEventListener("load", onLoad);

function onLoad(event) {
  initWebSocket();
  getControls();

  messageAnalog[0] = focusSlider.value;
  messageAnalog[1] = irisSlider.value;
  messageAnalog[2] = zoomSlider.value;
  //messageAnalog[3] = ;
  //messageAnalog[4] = ;
}

function getControls(){
  focusSlider = document.getElementById("focus");
  focusLabel = document.getElementById("focusText");
  irisSlider = document.getElementById("iris");
  irisLabel = document.getElementById("irisText");
  zoomSlider = document.getElementById("zoom");
  zoomLabel = document.getElementById("zoomText");
}


function setFocus(value){
  focusSlider.value = value;
  focusLabel.innerHTML = value;
}

function setIris(value){
  irisSlider.value = value;
  irisLabel.innerHTML = value;
}

function setZoom(value){
  zoomSlider.value = value;
  zoomLabel.innerHTML = value;
}