#include <Arduino.h>

// Libraries for ESP-NOW protocol
#include <ESP8266WiFi.h>
#include <espnow.h>

// Libraries for A/D module
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// Library with global types and constants for RX and TX
#include "../../PanCamRemote_RX/src/PanCamRemote_types.h"

const bool DEBUG = false;

Adafruit_ADS1115 ads;


// setting GPIO properties
const uint8_t LED_STATE = 16;//LED_BUILTIN;


void onSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.println("Status:");
  Serial.println(sendStatus);
}

uint8_t peer1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Message for ESP-NOW communication
message lensMessage;

void setup(void) {
  Serial.begin(9600);
  while (!Serial)
    delay(10); // will pause until serial console opens

  pinMode(LED_STATE, OUTPUT);
  digitalWrite(LED_STATE, HIGH); // turn LED off
  
  WiFi.mode(WIFI_STA);
  // Get Mac Add
  Serial.print("Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("ESP-Now Sender");

  // Initializing the ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Problem during ESP-NOW init");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  // Register the peer
  Serial.println("Registering a peer");
  esp_now_add_peer(peer1, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  
  // Register a callback function in DEBUG mode
  if (DEBUG) {
    Serial.println("Registering send callback function");
    esp_now_register_send_cb(onSent);
  }

  // Boot finished
  digitalWrite(LED_STATE, LOW); // turn LED on

  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");
 
  ads.begin();
}

void loop() {
  //int16_t adc0, adc1, adc2, adc3;
  lensMessage.focus = map(ads.readADC_SingleEnded(0), 100, 17300, 0, 1000);
  lensMessage.iris = map(ads.readADC_SingleEnded(1), 100, 17300, 0, 1000);
  lensMessage.zoom = map(ads.readADC_SingleEnded(2), 100, 17300, 0, 1000);
  lensMessage.autoIris = false;
  lensMessage.record = false;

  //adc3 = ads.readADC_SingleEnded(3);

  if (DEBUG) {
    Serial.print("Focus: ");
    Serial.println(lensMessage.focus);
    Serial.print("Iris: ");
    Serial.println(lensMessage.iris);
    Serial.print("Zoom: ");
    Serial.println(lensMessage.zoom);
    Serial.print("Auto-Iris: ");
    Serial.println(lensMessage.autoIris);
    Serial.print("Record: ");
    Serial.println(lensMessage.record);
    Serial.println("Send a new message");
    Serial.println(" ");
  }

  esp_now_send(NULL, (uint8_t *) &lensMessage, sizeof(lensMessage));
}