#include "rf.h"

uint32_t count = 0;

uint8_t readBuff[MESSAGE_BUFF_LEN];
header messageInfo;

uint8_t rf_specific_settings[] = {
	CC1101_REGS(gdo_config[2]), 0x07,   // GDO_0 - Assert on CRC OK | Disable temp sensor
	CC1101_REGS(gdo_config[0]), 0x2E,   // GDO_2 - FIXME : do something usefull with it for tests
	CC1101_REGS(pkt_ctrl[0]), 0x0F,     // Accept all sync, CRC err auto flush, Append, Addr check and Bcast
};

const struct pio cc1101_cs_pin = LPC_GPIO_0_15;
const struct pio cc1101_miso_pin = LPC_SSP0_MISO_PIO_0_16;
const struct pio cc1101_gdo0 = LPC_GPIO_0_6;
const struct pio cc1101_gdo2 = LPC_GPIO_0_7;

volatile uint8_t check_rx = 0;
volatile uint8_t rx_done = 0;


// Calback when reciving a message via RF
void rf_rx_calback(uint32_t gpio)
{
	check_rx = 1;
}

// Calculates the CRC of a char array (not used)
uint32_t rc_crc32(uint32_t crc, const uint8_t *buf, size_t len)
{
	static uint32_t table[256];
	static int have_table = 0;
	uint32_t rem;
	uint8_t octet;
	int i, j;
	const uint8_t *p, *q;

	/* This check is not thread safe; there is no mutex. */
	if (have_table == 0)
	{
		/* Calculate CRC table. */
		for (i = 0; i < 256; i++)
		{
			rem = i; /* remainder from polynomial division */
			for (j = 0; j < 8; j++)
			{
				if (rem & 1)
				{
					rem >>= 1;
					rem ^= 0xedb88320;
				}
				else
					rem >>= 1;
			}
			table[i] = rem;
		}
		have_table = 1;
	}

	crc = ~crc;
	q = buf + len;
	for (p = buf; p < q; p++)
	{
		octet = *p; /* Cast to unsigned octet. */
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
	}
	return ~crc;
}

void radio_stuff(){

	uint8_t status = 0;

	do	// Do not leave radio in an unknown or unwated state
	{
		status = (cc1101_read_status() & CC1101_STATE_MASK);
	} while (status == CC1101_STATE_TX);
	if (status != CC1101_STATE_RX)
	{
		static uint8_t loop = 0;
		loop++;
		if (loop > 10)
		{
			if (cc1101_rx_fifo_state() != 0)
			{
				cc1101_flush_rx_fifo();
			}
			cc1101_enter_rx_mode();
			loop = 0;
		}
	}
}

// Check a recieved packet if it need to be interpreted
uint8_t checkPacket(uint8_t *data)
{

	header mHeader;
	memcpy(&mHeader,data,sizeof(header));
	uint32_t rxCount;
	memcpy(&rxCount,data+sizeof(header),sizeof(uint32_t));

#ifdef NO_FILTER_DISPLAY
	uprintf(UART0, "Check packet:\n\r - destination: %x\n\r - source: %x\n\r - net id: %x\n\r - mtype: %d\n\r",
		mHeader.dest,
		mHeader.src,
		mHeader.netid,
		mHeader.mtype_nbpay >> 2
	);
	uprintf(UART0, " - nb blocs: %d\n\r - count: %d\n\r - nb packet: %d\n\r - id packet: %d\n\r",
		mHeader.mtype_nbpay & 0x03,
		rxCount,
		mHeader.nbpacket,
		mHeader.idpacket
	);
#endif

	//Check Source & destination
	if (mHeader.dest != DEVICE_ADDRESS && mHeader.dest != BROADCAST) // Checks if it is the device's address
		return 0;
	if (mHeader.src != LINKED_ADDRESS) // Checks if it is the neighbour's address
		return 0;

	if (mHeader.netid != NETID) // Check NetworkID
		return 0;

	return 1;
}

// RF config
void rf_config(void)
{
	config_gpio(&cc1101_gdo0, LPC_IO_MODE_PULL_UP, GPIO_DIR_IN, 0);
	cc1101_init(0, &cc1101_cs_pin, &cc1101_miso_pin); // ssp_num, cs_pin, miso_pin
	cc1101_config();
	cc1101_update_config(rf_specific_settings, sizeof(rf_specific_settings));
	set_gpio_callback(rf_rx_calback, &cc1101_gdo0, EDGE_RISING);
	cc1101_set_address(DEVICE_ADDRESS);
#ifdef DEBUG
	uprintf(UART0, "CC1101 RF link init done.\n\r");
#endif
}

void handle_rf_rx_data()
{
	uint8_t data[RF_BUFF_LEN];
	uint8_t status = 0;

	// Check for received packet (and get it if any)
#ifdef NO_FILTER_DISPLAY
	uint8_t ret = 0;
	ret = cc1101_receive_packet(data, RF_BUFF_LEN, &status);	
	uprintf(UART0, "Message recived (%d)\n\r",ret);
#else
	cc1101_receive_packet(data, RF_BUFF_LEN, &status);
#endif


	// Go back to RX mode
	cc1101_enter_rx_mode();

	if (checkPacket(data)) // Checks packet
	{

		header mHeader;
		memcpy(&mHeader,data,sizeof(header));
		uint32_t rxCount;
		memcpy(&rxCount,data+sizeof(header),sizeof(uint32_t));

		uint8_t payloadLen = (mHeader.mtype_nbpay & 0x03) * PAYLOAD_BLOC_LEN;
		uprintf(UART0,"Packet Checked from %x, count: %d, payload is %d bytes long, nb %d of %d:\n\r",mHeader.src,rxCount,payloadLen,(mHeader.idpacket+1),(mHeader.nbpacket+1));

		memcpy(readBuff+mHeader.idpacket*MAX_NB_BLOCK*PAYLOAD_BLOC_LEN,data+sizeof(header)+sizeof(uint32_t),(mHeader.mtype_nbpay&0x03)*PAYLOAD_BLOC_LEN);

		uint8_t messageDone = 0;
		if(mHeader.idpacket==mHeader.nbpacket){
			uprintf(UART0,"Message is complete\n\r");
			messageInfo = mHeader;
			messageDone = 1;
		}
		rx_done = messageDone;
	}
}

void send_on_rf(uint8_t *payload, uint16_t nbPayload, uint16_t messageLen, uint8_t destination, uint8_t msgType)
{
	uint8_t nbPacketToSend = nbPayload / MAX_NB_BLOCK;
	uint8_t nbBlocLast = nbPayload % MAX_NB_BLOCK + 1;
	if(nbBlocLast == 0) nbBlocLast = MAX_NB_BLOCK;
	uint8_t i, j;

	header mHeader;
	mHeader.dest = destination;
	mHeader.src = DEVICE_ADDRESS;
	mHeader.netid = NETID;
	mHeader.lastPacketLen = messageLen % (PAYLOAD_BLOC_LEN*MAX_NB_BLOCK);

	uint8_t tx_data[BUFF_LEN];

	for(i=0;i<=nbPacketToSend;i++){

		if(i<nbPacketToSend){
			mHeader.packetLen = sizeof(header)+sizeof(uint32_t)+1+(MAX_NB_BLOCK*PAYLOAD_BLOC_LEN);
			mHeader.mtype_nbpay = msgType << 2 | MAX_NB_BLOCK;
		} else {
			mHeader.packetLen = sizeof(header)+sizeof(uint32_t)+1+(nbBlocLast*PAYLOAD_BLOC_LEN);
			mHeader.mtype_nbpay = msgType << 2 | nbBlocLast;

		}

		mHeader.nbpacket = nbPacketToSend;
		mHeader.idpacket = i;

		memcpy(tx_data,&mHeader,sizeof(header));
		memcpy(tx_data+sizeof(header),&count,sizeof(uint32_t));

		for(j=0;j<(mHeader.mtype_nbpay & 0x3);j++){
			memcpy(tx_data+sizeof(header)+sizeof(uint32_t)+j*PAYLOAD_BLOC_LEN,payload+(i*MAX_NB_BLOCK+j)*PAYLOAD_BLOC_LEN,PAYLOAD_BLOC_LEN);
		}

		// Send   
		if (cc1101_tx_fifo_state() != 0)
		{
			cc1101_flush_tx_fifo();
		}

		int ret = cc1101_send_packet(tx_data, mHeader.packetLen+1);

#ifdef DEBUG	
		uprintf(UART0, "Message sent: %d (%x to %x)\n\r",ret,mHeader.src,mHeader.dest);
#endif
		msleep(250);
	}
	count++;
}
