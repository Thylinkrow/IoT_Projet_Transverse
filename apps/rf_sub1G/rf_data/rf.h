#include "lib/stdio.h"
#include "drivers/serial.h"
#include "drivers/gpio.h"
#include "extdrv/cc1101.h"
#include "core/system.h"

#define RF_BUFF_LEN 64
#define MESSAGE_BUFF_LEN 512
#define BUFF_LEN 60
#define MAX_NB_BLOCK 3
#define SELECTED_FREQ FREQ_SEL_48MHz
#define BROADCAST 0xff
#define PAYLOAD_BLOC_LEN 16

#define DEVICE_ADDRESS 0x33 // Adress
#define LINKED_ADDRESS 0x34 // Address of the gateway
#define NETID 0x66          // Network id

#define UPLINK 0x04
#define DOWNLINK 0x01
#define CRC_LENGTH 1

#define DEBUG 1

uint8_t rc_crc8(uint8_t *data, size_t len);

void radio_stuff();

uint8_t checkPacket(uint8_t *data);
void rf_config(void);

void handle_rf_rx_data();
void send_on_rf(uint8_t *payload, uint16_t nbPayload, uint16_t messageLen, uint8_t destination, uint8_t msgType);

struct header{
	uint8_t packetLen;
	uint8_t dest;
	uint8_t src; 
	uint8_t netid;
	uint8_t mtype_nbpay;
	uint8_t idpacket;
	uint8_t nbpacket;
	uint8_t lastPacketLen;
};
typedef struct header header;

extern uint8_t rf_specific_settings[];
extern volatile uint8_t check_rx;
extern volatile uint8_t rx_done;
extern uint8_t readBuff[];
extern header messageInfo;