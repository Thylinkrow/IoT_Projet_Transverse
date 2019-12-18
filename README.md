# IoT Transverse Project

## Contributors
* Gabriel Bailly
* Loïc Caille

## Description

### Security and Integrity
The messages are identified and checked
* Destination
* Source
* Network ID
* Type of device concerned (Gateway, Aggregator or Sensor)

The messages are encrypted (using *pseudo AES encryption* implementing *Sub byte*, *Rotate byte* and *Add round key*)
The integrity of each messages is checked using a CRC

### Sending messages spread on multiple packets
The 64 bytes limit has then been increased to 512 bytes
Each packet is identified within a message
When a packet is missing the recieving device can ask 3 times the sender to resend the missing packets