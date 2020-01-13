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

**Packet header**  
![header](https://raw.githubusercontent.com/Thylinkrow/IoT_Projet_Transverse/master/header.png)

The messages are encrypted (using [tiny AES](https://github.com/kokke/tiny-AES-c) by [kokke](https://github.com/kokke)).
The integrity of each messages is checked using a CRC

### Sending messages spread on multiple packets
The 64 bytes limit has then been increased to 512 bytes.
Each packet is identified within a message.
When a packet is missing the recieving device can ask 3 times the sender to resend the missing packets.

**Packet reconstruction**  
![packet](https://raw.githubusercontent.com/Thylinkrow/IoT_Projet_Transverse/master/packet.png)

### Activity diagram
![activity diagram](https://raw.githubusercontent.com/Thylinkrow/IoT_Projet_Transverse/master/IoT_Activity_Diagram.png)
