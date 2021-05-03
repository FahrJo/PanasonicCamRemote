# PanCamRemote_RX

An ESP32 based remote control receiver for the Panasonic camcorder remote interface. It contains a web interface and also provides an API to receive commands by another embedded controller over WiFi.

## REST-API

Basic commands can be sent via a REST-API (HTTP GET). After connecting to the AP (SSID/PW: PANCAM_REMOTE), the API is available at the URL http://cam.local/.

`<URL>/<KEY>?<PARAM>=<VALUE>`
| KEY | PARAM | VALUE |
|-----|-------|-------|
|focus|value  |0-1000 |
|iris |value  |0-1000 |
|zoom |value  |0-1000 |
|autoIris|value|true / false|
|record|value |true / false|

<br/>

## HTML Interface

One solution to remote control the camera is via the web interface, hosted on the ESP32. After connecting to the AP (SSID/PW: PANCAM_REMOTE), the HTML page can be found under http://cam.local/. The interface is usable as PWA for direct access from home screen. The web interface is connected via websocket, but it also uses the REST-API as fallback.

<p align="center">
<img src="img/gui-view_web.png" width="50%">
</p>

## ESP-NOW Interface

The device can also be controled over the ESP-NOW protocol. A project for a hardware remote sender, using this interface, can be found here: https://github.com/FahrJo/PanCamRemote_TX.