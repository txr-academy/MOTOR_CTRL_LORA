#include <SPI.h>
#include <LoRa.h>
#include "esp_sleep.h"
#include "driver/rtc_io.h"

// -------- Pins --------
#define SS      5
#define RST     14
#define DIO0    26
#define BUTTON_PIN    33   
#define LED_PIN       27   
#define RED_LED_PIN   12   
#define BUILTIN_LED    2   
void setupLoRa() {
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(8);
  LoRa.enableCrc();
}

// -------- Protocol --------
#define DEVICE_ID    0x01
#define CMD_PUMP     0x01
#define CMD_STATUS   0x02
#define ARG_ON       0x01
#define ARG_OFF      0x00
#define ACK_CODE     0xAA
#define ACK_TIMEOUT  1200
#define MAX_RETRIES  4

RTC_DATA_ATTR bool lastPumpState = false; 
RTC_DATA_ATTR bool commError = false; 

byte checksum(byte a, byte b, byte c) { return a ^ b ^ c; }

void updateLeds(bool pumpOn, bool errorOn) {
    rtc_gpio_hold_dis((gpio_num_t)LED_PIN);
    rtc_gpio_hold_dis((gpio_num_t)RED_LED_PIN);
    pinMode(LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, pumpOn ? HIGH : LOW);
    digitalWrite(RED_LED_PIN, errorOn ? HIGH : LOW);
    rtc_gpio_set_direction((gpio_num_t)LED_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level((gpio_num_t)LED_PIN, pumpOn ? 1 : 0);
    rtc_gpio_set_direction((gpio_num_t)RED_LED_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level((gpio_num_t)RED_LED_PIN, errorOn ? 1 : 0);
    rtc_gpio_hold_en((gpio_num_t)LED_PIN);
    rtc_gpio_hold_en((gpio_num_t)RED_LED_PIN);
}

bool waitForAck() {
    unsigned long start = millis();
    while (millis() - start < ACK_TIMEOUT) {
        int packetSize = LoRa.parsePacket();
        if (packetSize >= 4) {
            byte id    = LoRa.read();
            byte ack   = LoRa.read();
            byte rState = LoRa.read(); 
            byte chk   = LoRa.read();
            if (id == DEVICE_ID && ack == ACK_CODE && chk == checksum(id, ack, rState)) {
                lastPumpState = (rState == ARG_ON); 
                Serial.print(">>> ACK RECEIVED: Pump is ");
                Serial.println(lastPumpState ? "ON ✔" : "OFF ✔");
                return true; 
            }
        }
    }
    Serial.println(">>> NO ACK RECEIVED ❌");
    return false;
}

void sendCommand(byte cmd, byte arg) {
    byte chk = checksum(DEVICE_ID, cmd, arg);
    LoRa.beginPacket();
    LoRa.write(DEVICE_ID);
    LoRa.write(cmd);
    LoRa.write(arg);
    LoRa.write(chk);
    LoRa.endPacket();
    LoRa.receive(); 
}


void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, HIGH);

    SPI.begin(18, 19, 23, SS);
    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("LoRa Init Failed!");
        updateLeds(lastPumpState, true);
        esp_deep_sleep_start();
    }

    Serial.println("--- Transmitter Woke Up ---");

    // 1. SYNC: Fetch current state from receiver
    Serial.println("Requesting Status Sync...");
    bool syncSuccess = false;
    for(int i=0; i<MAX_RETRIES; i++) {
        sendCommand(CMD_STATUS, 0x00);
        if(waitForAck()) {
            syncSuccess = true;
            commError = false;
            break;
        }
        delay(150);
    }

    // 2. LOGIC: Compare physical switch to receiver state
    bool currentSwitch = (digitalRead(BUTTON_PIN) == LOW); 
    Serial.print("Physical Switch is currently: ");
    Serial.println(currentSwitch ? "ON" : "OFF");
    
    if (syncSuccess) {
        if (currentSwitch != lastPumpState) {
            Serial.println("Mismatch Detected! Correcting Receiver...");
            for (int i = 0; i < MAX_RETRIES; i++) {
                sendCommand(CMD_PUMP, currentSwitch ? ARG_ON : ARG_OFF);
                if (waitForAck()) break;
                delay(150);
            }
        } else {
            Serial.println("Switch and Receiver are already in sync.");
        }
    } else {
        Serial.println("Sync Failed - Comm Error.");
        commError = true; 
    }

    // 3. SLEEP
    updateLeds(lastPumpState, commError);
    Serial.println("Entering Deep Sleep...");
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, currentSwitch ? 1 : 0);
    digitalWrite(BUILTIN_LED, LOW);
    delay(100);
    esp_deep_sleep_start();
}

void loop() {}
