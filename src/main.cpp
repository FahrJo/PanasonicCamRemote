#include <Arduino.h>

// Libraries for web server
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <LITTLEFS.h>

// Library for ESP-NOW protocol
#include <esp_now.h>

// Libraries for D/A module
#include <Wire.h>
#include <Adafruit_MCP4728.h>

// Library with global types and constants for RX and TX
#include "PanCamRemote_types.h"

const bool DEBUG = false;

Adafruit_MCP4728 mcp;

// Message for ESP-NOW communication
message lensMessage;

output_image outputImage;

// Replace with your network credentials
const char* ssid = "PANCAM_REMOTE";
const char* password = "PANCAM_REMOTE";

String focusValue = "0";
String irisValue = "0";
String zoomValue = "500";

const int output = 2;

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

const char* PARAM_INPUT = "value";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Replaces placeholder with button section in your web page
String processor(const String& var){
  Serial.println(var);
  if (var == "FOCUSVALUE"){
    return focusValue;
  }
  if (var == "IRISVALUE"){
    return irisValue;
  }
  if (var == "ZOOMVALUE"){
    return zoomValue;
  }
  return String();
}


void onDataReceiver(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // We don't use mac to verify the sender
  // Let us transform the incomingData into our message structure
  memcpy(&lensMessage, incomingData, sizeof(lensMessage));

  if (DEBUG) {
    Serial.println("Message received.");
    Serial.print("Focus:");
    Serial.println(lensMessage.focus); 
    Serial.print("Iris:");
    Serial.println(lensMessage.iris);
    Serial.print("Zoom:");
    Serial.println(lensMessage.zoom);
    Serial.print("Auto-Iris:");
    Serial.println(lensMessage.autoIris);
    Serial.print("Record:");
    Serial.println(lensMessage.record);
  }
  outputImage.focus = lensMessage.focus;
  outputImage.iris = lensMessage.iris;
  outputImage.zoom = lensMessage.zoom;
}



void setup(void) {
  Serial.begin(9600);
  while (!Serial)
    delay(10); // will pause until serial console opens


  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(output, ledChannel);
  
  // Initialize LittleFS
  if(!LITTLEFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // Try to initialize!
  if (!mcp.begin()) {
    Serial.println("Failed to find MCP4728 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MCP4728 Found!");

  // set initial values to analog outputs
  // value is vref/2, with 2.048V internal vref and *2X gain*
  mcp.setChannelValue(MCP4728_CHANNEL_A, focusValue.toInt()*2, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
  mcp.setChannelValue(MCP4728_CHANNEL_B, irisValue.toInt()*2, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
  mcp.setChannelValue(MCP4728_CHANNEL_C, zoomValue.toInt()*2.65, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);

  mcp.saveToEEPROM();

    // Connect to Wi-Fi
  //WiFi.begin(ssid, password);
  //while (WiFi.status() != WL_CONNECTED) {
  //  delay(1000);
  //  Serial.println("Connecting to WiFi..");
  //}
  WiFi.mode(WIFI_STA);
  // Get Mac Add
  Serial.print("Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("ESP32 ESP-Now Broadcast");

  // Initializing the ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Problem during ESP-NOW init");
    return;
  }
  esp_now_register_recv_cb(onDataReceiver);


  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  if (!MDNS.begin("cam")) {
      Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);


  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LITTLEFS, "/index.html", String(), false, processor);
  });

  // Route for stylesheet
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LITTLEFS, "/style.css", "text/css");
    if (DEBUG) Serial.println("style.css");
  });

  // Route for JS file
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LITTLEFS, "/script.js", "text/js");
    if (DEBUG) Serial.println("script.js");
  });

  // Routes for favicons
  server.on("/apple-icon-120x120.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LITTLEFS, "/icon/apple-icon-120x120.png", "image/png");
    if (DEBUG) Serial.println("apple-icon-120x120.png");
  });
  server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LITTLEFS, "/icon/favicon.png", "image/png");
    if (DEBUG) Serial.println("favicon.png");
  });

  

  // Send a GET request for heartbeat
  server.on("/ping", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (DEBUG) Serial.println("Ping received");
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/focus?value=<inputMessage>
  server.on("/focus", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      focusValue = inputMessage;
      mcp.setChannelValue(MCP4728_CHANNEL_A, focusValue.toInt()*2, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
    }
    else {
      inputMessage = "No message sent";
    }
    if (DEBUG) Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
  
  // Send a GET request to <ESP_IP>/iris?value=<inputMessage>
  server.on("/iris", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input2 value on <ESP_IP>/iris?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      irisValue = inputMessage;
      mcp.setChannelValue(MCP4728_CHANNEL_B, irisValue.toInt()*2, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
    }
    else {
      inputMessage = "No message sent";
    }
    if (DEBUG) Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/zoom?value=<inputMessage>
  server.on("/zoom", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input2 value on <ESP_IP>/zoom?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      zoomValue = inputMessage;
      mcp.setChannelValue(MCP4728_CHANNEL_C, zoomValue.toInt()*2.65, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
    }
    else {
      inputMessage = "No message sent";
    }
    if (DEBUG) Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
  // Start server
  server.begin();

  // Boot finished
  ledcWrite(ledChannel, 100);
}




void loop() { 
  mcp.setChannelValue(MCP4728_CHANNEL_A, outputImage.focus * 2, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
  mcp.setChannelValue(MCP4728_CHANNEL_B, outputImage.iris * 2, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
  mcp.setChannelValue(MCP4728_CHANNEL_C, outputImage.zoom * 2.65, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
}


// void setup(){
//   // Serial port for debugging purposes
//   Serial.begin(115200);
  
//   // configure LED PWM functionalitites
//   ledcSetup(ledChannel, freq, resolution);
  
//   // attach the channel to the GPIO to be controlled
//   ledcAttachPin(output, ledChannel);
  
//   ledcWrite(ledChannel, sliderValue.toInt());


// }