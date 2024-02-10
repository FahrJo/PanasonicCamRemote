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

#define DEBUG       false
#define PARAM_INPUT "value"


Adafruit_MCP4728 mcp;

// Message for ESP-NOW communication
message messageWs;
message messageEsp;
message messageHttp;

message outputImage;
message outputImageDefault = {0, 0, 500, 0, 1000}; // focus, iris, zoom, focus_ll, focus_ul

// Replace with your network credentials
const char* ssid = "PANCAM_REMOTE";
const char* password = "PANCAM_REMOTE";

boolean exec_ota_flag = false;

const int LedState = 2;
const int OUT_RECORD = 16;
const int OUT_AUTOIRIS = 17;

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;


unsigned long lastExecutionTime = 0;
unsigned long delayTime = 1000;


void setOutputImage(protocolType type) {
  switch (type) {
    case WS:      outputImage = messageWs;
                  break;
    case ESP_NOW: outputImage = messageEsp;
                  break;
    case HTTP:    outputImage = messageHttp;
                  break;
    default:      outputImage = outputImageDefault;
                  break;
  }

  // remap values for higher resolution of hmi controlls
  uint16_t focus =  map(outputImage.focus, 0, 1000, outputImage.focus_ll, outputImage.focus_ul);

  digitalWrite(OUT_AUTOIRIS, !outputImage.autoIris);
  digitalWrite(OUT_RECORD, outputImage.record);

  // Send config data only in INIT mode. Otherwise use the fast write methode
  if (type == INIT) {
  // value is vref/2, with 2.048V internal vref and *2X gain*
  mcp.setChannelValue(MCP4728_CHANNEL_A, focus * 2, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
  mcp.setChannelValue(MCP4728_CHANNEL_B, outputImage.iris * 2, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
  mcp.setChannelValue(MCP4728_CHANNEL_C, outputImage.zoom * 2.65, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
  }
  else{
    mcp.fastWrite(focus * 2, outputImage.iris * 2, outputImage.zoom * 2.65, 0);
  }
}


/****************************
 * Websocket functions
 */
#pragma region websocket
// Create AsyncWebServer object on port 80 and a websocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


bool validateData(message *message){
  return ((message->focus >= 0) && (message->focus <= 1000) && (message->iris >= 0) && (message->iris <= 1000) && (message->zoom >= 0) && (message->zoom <= 1000) && (message->focus_ll >= 0) && (message->focus_ll <= 1000) && (message->focus_ul >= 0) && (message->focus_ul <= 1000));
}


void notifyClients(uint8_t *data, size_t len) {
  ws.binaryAll(data, len);
}

void notifyClients(uint8_t *data, size_t len, AsyncWebSocketClient *sender){
  for( auto client : ws.getClients()){
    if (sender != client){
        client->binary(data, len);
    }
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *sender) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_BINARY) {
    
    if (DEBUG) {
      Serial.print("WS: ");
      for(int i=0; i < len; i+=2) {
        Serial.print(*(uint16_t *) &data[i]);
        Serial.print("|");
      }
      Serial.println();
    }

    messageWs = *(message *)data;
    if (validateData(&messageWs)){
      // Notify all websocket clients except sender
      notifyClients(data, len, sender);
      setOutputImage(WS);
    }
  }
  delay(1);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len, client);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
#pragma endregion


// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if (var == "FOCUSVALUE"){
    return String(outputImage.focus);
  }
  else if (var == "IRISVALUE"){
    return String(outputImage.iris);
  }
  else if (var == "ZOOMVALUE"){
    return String(outputImage.zoom);
  }
  else if (var == "FOCUSVALUE_LL"){
    return String(outputImage.focus_ll);
  }
  else if (var == "FOCUSVALUE_UL"){
    return String(outputImage.focus_ul);
  }
  else if (var == "AUTOIRIS") {
    return (outputImage.autoIris) ? "checked" : "";
  }
  return String();
}


void handleEspNowMessage(const uint8_t * mac, const uint8_t *data, int len) {
  // We don't use mac to verify the sender
  // Let us transform the data into our message structure
  //memcpy(&messageEsp, data, sizeof(messageEsp));
  messageEsp = *(message *)data;

  if (DEBUG) {
      Serial.print("EN: ");
      for(int i=0; i < len; i+=2) {
        Serial.print(*(uint16_t *) &data[i]);
        Serial.print("|");
      }
      Serial.println();
    }
  
  if (validateData(&messageWs)){
    // Notify all websocket clients
    notifyClients((uint8_t *)data, len);
    setOutputImage(ESP_NOW);  
  }
  delay(1);
}

void initGPIO(){
    // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  pinMode(OUT_AUTOIRIS, OUTPUT);
  pinMode(OUT_RECORD, OUTPUT);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(LedState, ledChannel);

  digitalWrite(OUT_AUTOIRIS, LOW);
  digitalWrite(OUT_RECORD, HIGH);
}

void initFS(){
  // Initialize LittleFS
  if(!LITTLEFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
}

void initDAC(){
  // Try to initialize!
  if (!mcp.begin()) {
    Serial.println("Failed to find MCP4728 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MCP4728 Found!");

  // set initial values to analog outputs
  setOutputImage(INIT);

  mcp.saveToEEPROM();
}

void initWiFi(){
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
  esp_now_register_recv_cb(handleEspNowMessage);


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

  initWebSocket();
}

void initWebServer(){
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LITTLEFS, "/index.html", String(), false, processor);
  });


  /******************************************/
  /* REST-API for HTTP access to the server */
  /******************************************/

  #pragma region REST-API
  // Route for stylesheet
  server.on("/style.css", HTTP_GET, [] (AsyncWebServerRequest *request){
    request->send(LITTLEFS, "/style.css", "text/css");
    if (DEBUG) Serial.println("style.css");
  });

  // Route for JS file
  server.on("/script.js", HTTP_GET, [] (AsyncWebServerRequest *request){
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

  
  // Send a GET request for ping
  server.on("/ping", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // GET input1 value on <ESP_IP>/ping?value=<inputMessage>
    if (DEBUG) Serial.println("Ping received");
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/focus?value=<inputMessage>
  server.on("/focus", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/focus?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      messageHttp = outputImage;
      messageHttp.focus = request->getParam(PARAM_INPUT)->value().toInt();
      
      setOutputImage(HTTP);
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
      messageHttp = outputImage;
      messageHttp.iris = request->getParam(PARAM_INPUT)->value().toInt();

      setOutputImage(HTTP);
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
      messageHttp = outputImage;
      messageHttp.zoom = request->getParam(PARAM_INPUT)->value().toInt();

      setOutputImage(HTTP);
    }
    else {
      inputMessage = "No message sent";
    }
    if (DEBUG) Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/autoIris?value=<inputMessage>
  server.on("/autoIris", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input2 value on <ESP_IP>/autoIris?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      messageHttp = outputImage;
      messageHttp.autoIris = (request->getParam(PARAM_INPUT)->value() == "true");

      setOutputImage(HTTP);
    }
    else {
      inputMessage = "No message sent";
    }
    if (DEBUG) Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/record?value=<inputMessage>
  server.on("/record", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input2 value on <ESP_IP>/record?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      messageHttp = outputImage;
      messageHttp.record = (request->getParam(PARAM_INPUT)->value() == "true");

      setOutputImage(HTTP);
    }
    else {
      inputMessage = "No message sent";
    }
    if (DEBUG) Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  #pragma endregion

  // Start server
  server.begin();
}


void setup(void) {
  Serial.begin(9600);
  while (!Serial)
    delay(10); // will pause until serial console opens

  // Mount file system
  initFS();

  // Initialize GPIO
  initGPIO();

  // Initialize external DAC
  initDAC();

  // Initialize WiFi and ESP-NOW
  initWiFi();

  // Initialize webserver and REST-API
  initWebServer();

  // Boot finished
  ledcWrite(ledChannel, 50);
}


void loop() { 
  if (millis() > lastExecutionTime + delayTime){
    lastExecutionTime = millis();

    ws.cleanupClients();
  }
}