#include <SPI.h>
#include <LoRa.h>

#define SS     5
#define RST    14
#define DIO0   26
#define RELAY_PIN 25
void setupLoRa() {
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(8);
  LoRa.enableCrc();
}
#define RELAY_ON    LOW   
#define RELAY_OFF   HIGH
#define DEVICE_ID    0x01
#define CMD_PUMP     0x01
#define CMD_STATUS   0x02
#define ARG_ON       0x01
#define ARG_OFF      0x00
#define ACK_CODE     0xAA

byte checksum(byte a, byte b, byte c) { return a ^ b ^ c; }

void sendAck() {
    // Read the actual hardware level
    byte physicalState = (digitalRead(RELAY_PIN) == RELAY_ON) ? ARG_ON : ARG_OFF;
    byte chk = checksum(DEVICE_ID, ACK_CODE, physicalState);

    LoRa.idle();
    LoRa.beginPacket();
    LoRa.write(DEVICE_ID);
    LoRa.write(ACK_CODE);
    LoRa.write(physicalState); 
    LoRa.write(chk);
    LoRa.endPacket();
    LoRa.receive();

    Serial.print("<<< ACK SENT: Pump is ");
    Serial.println(physicalState == ARG_ON ? "ON" : "OFF");
}

void resetLoRa() {
  digitalWrite(RST, LOW);
  delay(10);
  digitalWrite(RST, HIGH);
  delay(10);
  LoRa.begin(433E6);
  LoRa.receive();
}

void setup() {
    Serial.begin(9600);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_OFF); 

    SPI.begin(18, 19, 23, SS);
    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("LoRa Init Failed!");
        while (1);
    }
    LoRa.receive();
    Serial.println("Receiver Ready - Awaiting Sync/Commands...");
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize == 4) {
        byte id  = LoRa.read();
        byte cmd = LoRa.read();
        byte arg = LoRa.read();
        byte chk = LoRa.read();

        if (id != DEVICE_ID || chk != checksum(id, cmd, arg)) {
            Serial.println("Received Invalid Packet");
            return;
        }

        if (cmd == CMD_PUMP) {
            if (arg == ARG_ON) {
                digitalWrite(RELAY_PIN, RELAY_ON);
             delay(50);
             resetLoRa();
              
                Serial.println("Command: Turning Pump ON");
            } else {
                digitalWrite(RELAY_PIN, RELAY_OFF);
                delay(50);
                resetLoRa();
                
                Serial.println("Command: Turning Pump OFF");
            }
            delay(50); 
            sendAck();
        } 
        else if (cmd == CMD_STATUS) {
            Serial.println("Command: Status Request Received");
            sendAck(); 
        }
    }
}
