## LORA BASED REMOTE CONTROLLED MOTOR

Long-range wireless system designed to control and monitor industrial motors in environments where Wi-Fi or Cellular signals are unavailable. 
This project demonstrates a complete end-to-end implementation of LoRa (Long Range) protocol for critical hardware triggering.

Features:
(1)Long-Range Communication: Reliable motor triggering at distances of 2km+ (Urban) and up to 10km (Line-of-Sight).
(2)Dual-Controller Architecture: * Transmitter: ESP32-based handheld/gateway controller.
                                 * Receiver   : ESP32-based unit with integrated motor driver logic.

                                 
System Architecture_

The system operates on a Point-to-Point (P2P) LoRa topology:

User Command: Dispatched via the ESP32 Transmitter.

RF Propagation: Sub-GHz LoRa modulation (433MHz).

Execution: The Receiver unit validates the packet and toggles the Relay/H-Bridge via ESP32 GPIOs.

Feedback: Success signal is transmitted back to the user interface.                                 

Hardware Stack

Component                   Specification

Microcontrollers            ESP32 (Transmitter/Reciever)

LoRa Module                 SX1278 LoRa Module

Motor Driver                Relay Module / Contactor

Indication                  LED-ON/OFF, Signal Loss


*Connections(ESP32+LORA)

NSS-  5   GPIO
MOSI- 23   "
MISO- 19   " 
SCK-  18   "
RST-  14   "
DIO0- 26   "



