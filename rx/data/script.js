let lock = false;

let messageAnalog = new Uint16Array(5); // 10 Byte
let messageDigital = new Uint8Array(6); //  6 Byte
let lastMessageTimestamp = 0;
let websocketOpen = false;

let upperLowerDelta = 150;

let focusSlider = undefined;
let focusLimitsContainer = undefined;
let focusSlider_ll = undefined;
let focusSlider_ul = undefined;
let focusLabel = undefined;
let irisSlider = undefined;
let irisLabel = undefined;
let zoomSlider = undefined;
let zoomLabel = undefined;
let extFocusSwitch = undefined;
let autoIrisSwitch = undefined;

function updateSlider(element, final = false) {
  if (!lock || final) {
    lock = true;
    var value = element.value;
    if (element.id == "focus") {
      messageAnalog[0] = value;
      focusLabel.innerHTML = value;
    } else if (element.id == "iris") {
      messageAnalog[1] = value;
      irisLabel.innerHTML = value;
    } else console.log("no place in message array");

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
    } else console.log("no place in message array");

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

function onExtendedFocusControl(element) {
  if (element.checked === true) {
    focusLimitsContainer.classList.remove("hide");
  } else {
    focusLimitsContainer.classList.add("hide");
  }
}

function onUpperSliderInput() {
  lowerVal = parseInt(focusSlider_ll.value);
  upperVal = parseInt(focusSlider_ul.value);
  lowerVal_new = lowerVal;
  upperVal_new = upperVal;

  if (upperVal < lowerVal + upperLowerDelta) {
    lowerVal_new = upperVal - upperLowerDelta;

    if (lowerVal == focusSlider_ll.min) {
      upperVal_new = upperLowerDelta;
    }
  }

  focusSlider_ll.value = lowerVal_new;
  focusSlider_ul.value = upperVal_new;

  messageAnalog[3] = lowerVal_new;
  messageAnalog[4] = upperVal_new;
  if (websocketOpen) {
    sendToWebsocket();
  }
}

function onLowerSliderInput() {
  lowerVal = parseInt(focusSlider_ll.value);
  upperVal = parseInt(focusSlider_ul.value);
  lowerVal_new = lowerVal;
  upperVal_new = upperVal;

  if (lowerVal > upperVal - upperLowerDelta) {
    upperVal_new = lowerVal + upperLowerDelta;

    if (upperVal == focusSlider_ul.max) {
      lowerVal_new = parseInt(focusSlider_ul.max) - upperLowerDelta;
    }
  }

  focusSlider_ll.value = lowerVal_new;
  focusSlider_ul.value = upperVal_new;

  messageAnalog[3] = lowerVal_new;
  messageAnalog[4] = upperVal_new;
  if (websocketOpen) {
    sendToWebsocket();
  }
}

function onAutoIrisSwitch(element) {
  messageDigital[0] = element.checked;

  if (element.checked) irisSlider.setAttribute("disabled", "disabled");
  else irisSlider.removeAttribute("disabled");

  if (websocketOpen) {
    sendToWebsocket();
  } else {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
      if (xhr.readyState == XMLHttpRequest.DONE) {
        lock = false;
      }
    };
    xhr.open("GET", "/" + element.id + "?value=" + element.checked, true);
    xhr.send();
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
    focusSlider.removeAttribute("disabled");
    !autoIrisSwitch.checked && irisSlider.removeAttribute("disabled");
    zoomSlider.removeAttribute("disabled");
    focusSlider_ll.removeAttribute("disabled");
    focusSlider_ul.removeAttribute("disabled");
    autoIrisSwitch.removeAttribute("disabled");
    autoIrisSwitch.nextElementSibling.classList.remove("disabled");
  } else {
    focusSlider.setAttribute("disabled", "disabled");
    irisSlider.setAttribute("disabled", "disabled");
    zoomSlider.setAttribute("disabled", "disabled");
    focusSlider_ll.setAttribute("disabled", "disabled");
    focusSlider_ul.setAttribute("disabled", "disabled");
    autoIrisSwitch.setAttribute("disabled", "disabled");
    autoIrisSwitch.nextElementSibling.classList.add("disabled");
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
  setFocusLimits(focus_ll, focus_ul);
  setAutoIris(autoIris);
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
  messageAnalog[3] = focusSlider_ll.value;
  messageAnalog[4] = focusSlider_ul.value;
  messageDigital[0] = autoIrisSwitch.checked;
}

function getControls() {
  focusSlider = document.getElementById("focus");
  focusLabel = document.getElementById("focusText");
  focusLimitsContainer = document.getElementById("focus-ext-container");
  focusSlider_ll = document.getElementById("focus_ll");
  focusSlider_ul = document.getElementById("focus_ul");
  irisSlider = document.getElementById("iris");
  irisLabel = document.getElementById("irisText");
  zoomSlider = document.getElementById("zoom");
  zoomLabel = document.getElementById("zoomText");
  extFocusSwitch = document.getElementById("extFocusSwitch");
  autoIrisSwitch = document.getElementById("autoIris");
}

function setFocus(value) {
  focusSlider.value = value;
  focusLabel.innerHTML = value;
}

function setIris(value) {
  irisSlider.value = value;
  irisLabel.innerHTML = value;
}

function setZoom(value) {
  zoomSlider.value = value;
  zoomLabel.innerHTML = value;
}

function setFocusLimits(value1, value2) {
  focusSlider_ll.value = value1;
  focusSlider_ul.value = value2;
}

function setAutoIris(auto) {
  autoIrisSwitch.checked = auto;
}
