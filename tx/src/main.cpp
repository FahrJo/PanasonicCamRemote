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

#define DEBUG false

Adafruit_ADS1115 ads;


// setting GPIO properties
const uint8_t LED_STATE = 16;//LED_BUILTIN;
const uint8_t SW_LIMIT = 12;
const uint8_t SW_AUTOIRIS = 13;
const uint8_t SW_RECORD = 14;


void onSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.println("Status:");
  Serial.println(sendStatus);
}

uint8_t peer1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint16_t focus_ll = 0;
uint16_t focus_ul = 1000;

const int16_t adc_ll = 0;
const int16_t adc_ul = 17400;

// delay loop for data sending
unsigned long lastExecutionTime = 0;
unsigned long delayTime = 25;

// Message for ESP-NOW communication
message lensMessage = {0, 0, 500, focus_ll, focus_ul}; // focus, iris, zoom, focus_ll, focus_ul


int16_t map_int(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max) {
  if (x < in_min) return out_min;
  else if (x > in_max) return out_max;
  else return (x - in_min) * (out_max - out_min) / (in_max - in_min + 1) + out_min;
}


void setup(void) {
  Serial.begin(9600);
  while (!Serial)
    delay(10); // will pause until serial console opens

  pinMode(LED_STATE, OUTPUT);
  digitalWrite(LED_STATE, HIGH); // turn LED off
  
  pinMode(SW_LIMIT, INPUT_PULLUP);
  pinMode(SW_AUTOIRIS, INPUT_PULLUP);
  pinMode(SW_RECORD, INPUT_PULLUP);

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
  if (millis() > lastExecutionTime + delayTime){
    lastExecutionTime = millis();
    lensMessage.focus = map_int(ads.readADC_SingleEnded(0), adc_ll, adc_ul, 0, 1000);
    lensMessage.iris = map_int(ads.readADC_SingleEnded(1), adc_ll, adc_ul, 0, 1000);
    lensMessage.zoom = map_int(ads.readADC_SingleEnded(2), adc_ll, adc_ul, 0, 1000);

    if (digitalRead(SW_LIMIT)) {
      focus_ul = map_int(ads.readADC_SingleEnded(3), adc_ll, adc_ul, 0, 1000);
    }
    else {
      focus_ll = map_int(ads.readADC_SingleEnded(3), adc_ll, adc_ul, 0, 1000);
    }
    lensMessage.focus_ll = focus_ll;
    lensMessage.focus_ul = focus_ul;
    
    lensMessage.autoIris = digitalRead(SW_AUTOIRIS);
    lensMessage.record = digitalRead(SW_RECORD);

    if (DEBUG) {
      Serial.print("Focus: ");
      Serial.println(lensMessage.focus);
      Serial.print("Iris: ");
      Serial.println(lensMessage.iris);
      Serial.print("Zoom: ");
      Serial.println(lensMessage.zoom);
      Serial.print("Auto-Iris: ");
      Serial.println(lensMessage.focus_ll);
      Serial.print("Record: ");
      Serial.println(lensMessage.focus_ul);
      Serial.println("Send a new message");
      Serial.println(" ");
    }

    esp_now_send(NULL, (uint8_t *) &lensMessage, sizeof(lensMessage));

    delay(1);
  }
}