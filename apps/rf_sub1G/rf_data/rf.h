#include "lib/stdio.h"
#include "drivers/serial.h"
#include "drivers/gpio.h"
#include "extdrv/cc1101.h"
#include "core/system.h"
#include "core/systick.h"

#define RF_BUFF_LEN 64
#define MESSAGE_BUFF_LEN 512
#define BUFF_LEN 60
#define MAX_NB_BLOCK 3
#define SELECTED_FREQ FREQ_SEL_48MHz
#define BROADCAST 0xff
#define PAYLOAD_BLOC_LEN 16
#define WAIT_RX_TIME 5000
#define QUEUE_SIZE 32
#define SEND_TIMEOUT 350

#define DEVICE_ADDRESS 0x34 // Adress
#define LINKED_ADDRESS 0x33 // Address of the gateway
#define NETID 0x66          // Network id

#define EVERYTYPE 	0b00 << 4
#define GATEWAYS 	0b01 << 4
#define AGGREGATORS 0b10 << 4
#define SENSORS 	0b11 << 4

#define HEARTBEAT 	0b0000
#define RQ_DATA		0b0001
#define TERMINATION 0b0010

#define FIRE_MNGMT 	0b0011

#define DEBUG 1

uint8_t rc_crc8(uint8_t *data, size_t len);

void radio_stuff();

uint8_t checkPacket(uint8_t *data);
void rf_config(void);

void handle_rf_rx_data();
void send_message(uint8_t *payload, uint16_t nbPayload, uint16_t messageLen, uint8_t destination, uint8_t msgType);

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

struct messageInfo{
	uint32_t count;
	uint32_t tick;
	uint32_t rxpack; 
	uint8_t asked;
	uint8_t addr;
	uint8_t nbpacket;
	uint8_t lastPacketLen;
	uint8_t msgType;
	uint8_t nbPayloadLast;
};
typedef struct messageInfo messageInfo;

struct sendJob{
	uint8_t* data;
	uint8_t idpacket;
};
typedef struct sendJob sendJob;

void initJobs();
uint8_t execJob();
uint8_t addJob(messageInfo info, uint8_t* data, uint8_t idpacket);
void send_on_rf();

extern uint8_t rf_specific_settings[];
extern volatile uint8_t check_rx;
extern volatile uint8_t rx_done;
extern header msgHeader;

extern uint8_t rxBuff[];
extern uint8_t txBuff[];
messageInfo rxMsgInfo;
messageInfo txMsgInfo;