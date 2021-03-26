#include <Arduino.h>

// Librarys for web server
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <LITTLEFS.h>

// Librarys for D/A module
#include <Adafruit_MCP4728.h>
#include <Wire.h>

const bool DEBUG = true;

Adafruit_MCP4728 mcp;

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


void setup(void) {
  Serial.begin(9600);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens


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

  // Print ESP Local IP Address
  //Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LITTLEFS, "/index.html", String(), false, processor);
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LITTLEFS, "/style.css", "text/css");
    if (DEBUG) Serial.println("style.css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LITTLEFS, "/script.js", "text/js");
    if (DEBUG) Serial.println("script.js");
  });
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




void loop() { }


// void setup(){
//   // Serial port for debugging purposes
//   Serial.begin(115200);
  
//   // configure LED PWM functionalitites
//   ledcSetup(ledChannel, freq, resolution);
  
//   // attach the channel to the GPIO to be controlled
//   ledcAttachPin(output, ledChannel);
  
//   ledcWrite(ledChannel, sliderValue.toInt());


// }