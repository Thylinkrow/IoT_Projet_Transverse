#include "core/system.h"
#include "core/systick.h"
#include "core/pio.h"
#include "lib/stdio.h"
#include "drivers/serial.h"
#include "drivers/gpio.h"
#include "drivers/ssp.h"
#include "extdrv/cc1101.h"
#include "extdrv/status_led.h"
#include "drivers/i2c.h"

#include "rf.h"
#include "encryption.h"
#include "uartInput.h"
#include "fire_management.h"

#define MODULE_VERSION 0x03
#define MODULE_NAME "RF Sub1G - USB"
#define RF_868MHz 1

#define DEBUG -1

// Pins configuration
const struct pio_config common_pins[] = {
	// UART
	{LPC_UART0_RX_PIO_0_1, LPC_IO_DIGITAL},
	{LPC_UART0_TX_PIO_0_2, LPC_IO_DIGITAL},
	// SPI
	{LPC_SSP0_SCLK_PIO_0_14, LPC_IO_DIGITAL},
	{LPC_SSP0_MOSI_PIO_0_17, LPC_IO_DIGITAL},
	{LPC_SSP0_MISO_PIO_0_16, LPC_IO_DIGITAL},
	// I2C 0
	{LPC_I2C0_SCL_PIO_0_10, (LPC_IO_DIGITAL | LPC_IO_OPEN_DRAIN_ENABLE)},
	{LPC_I2C0_SDA_PIO_0_11, (LPC_IO_DIGITAL | LPC_IO_OPEN_DRAIN_ENABLE)},
	ARRAY_LAST_PIO,
};

const struct pio status_led_green = LPC_GPIO_0_28;	// Green led
const struct pio status_led_red = LPC_GPIO_0_29;	// Red led
const struct pio button = LPC_GPIO_0_12; 			// ISP button

// Key for decrypting
uint8_t key[PAYLOAD_BLOC_LEN] = {0x63, 0x7c, 0x77, 0x7b, 0xf2, 0xa5, 0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3, 0x3b, 0xd6, 0x16};

// Initialisation vector for decrypting
uint8_t iv[PAYLOAD_BLOC_LEN] = {0xab, 0x4c, 0xd2, 0x26, 0x99, 0xff, 0x4d, 0x44, 0xa5, 0x5a, 0xfd, 0x32, 0x3f, 0xb4, 0xa2, 0xa2};

// TODO: remove
//uint8_t buff[512] = {"0,0,0\n0,1,0\n0,2,0\n0,3,0\n0,4,0\n0,5,0\n0,6,0\n0,7,0\n0,8,0\n0,9,0\n1,0,0\n1,1,0\n1,2,0\n1,3,0\n1,4,0\n1,5,0\n1,6,0\n1,7,0\n1,8,0\n1,9,0\n2,0,0\n2,1,0\n2,2,0\n2,3,0\n2,4,0\n2,5,0\n2,6,0\n2,7,0\n2,8,0\n2,9,0\n3,0,0\n3,1,0\n3,2,0\n3,3,0\n3,4,0\n3,5,0\n3,6,0\n3,7,0\n3,8,0\n3,9,0\n4,0,0\n4,1,0\n4,2,0\n4,3,0\n4,4,0\n4,5,0\n4,6,0\n4,7,0\n4,8,0\n4,9,0\n5,0,0\n5,1,0\n5,2,0\n5,3,0\n5,4,0\n5,5,0\n5,6,0\n5,7,0\n5,8,0\n5,9,0\n"};

uint8_t buff[512];

void loop();

/******************************************************************************/
void system_init()
{
	// Stop the watchdog
	startup_watchdog_disable(); // Do it right now, before it gets a chance to break in
	system_set_default_power_state();
	clock_config(SELECTED_FREQ);
	set_pins(common_pins);
	gpio_on();
	systick_timer_on(1); // 1 ms
	systick_start();
}

// Define our fault handler
void fault_info(const char *name, uint32_t len)
{
#if DEBUG > 0
	uprintf(UART0, name);
#endif
	while (1);
}

uint8_t chenillard_active = 1;

uint8_t chenillard_activation_request = 1;
void activate_chenillard(uint32_t gpio)
{
	if (chenillard_activation_request == 1)
	{
		uart_buff[0] = '0';
		uart_ptr = 1;
		uart_done = 1;
		chenillard_activation_request = 0;
	}
	else
	{
		uart_buff[0] = '1';
		uart_ptr = 1;
		uart_done = 1;
		chenillard_activation_request = 1;
	}
}

int main(void)
{
	system_init();
	uart_on(UART0, 115200, handle_uart_cmd);
	i2c_on(I2C0, I2C_CLK_100KHz, I2C_MASTER);
	ssp_master_on(0, LPC_SSP_FRAME_SPI, 8, 4 * 1000 * 1000); 		// bus_num, frame_type, data_width, rate
	status_led_config(&status_led_green, &status_led_red);

	rf_config();													// Radio init
	set_gpio_callback(activate_chenillard, &button, EDGE_RISING);	// Activate the chenillard on Rising edge (button release)

#if DEBUG > 0	
	uprintf(UART0, "App started\n\r");
	uprintf(UART0, "\t- Device address: %x\n\r\t- Linked address: %x\n\r",DEVICE_ADDRESS,LINKED_ADDRESS);
#endif

	memset(uart_buff,'0',UART_BUFF_LEN);
	memset(rxBuff,'0',MESSAGE_BUFF_LEN);

	// TODO: remove
	/*uint32_t nb_bloc = 360 / ENCRYPT_BLOCK_LEN;
	encrypt(test,key, iv,nb_bloc);
	send_message(test,nb_bloc,360,LINKED_ADDRESS,(EVERYTYPE|FIRE_MNGMT));*/

	while (1)
		loop();

	return 0;
}

void loop()
{

	if (chenillard_active == 1) // Verify that chenillard is enable
	{
		chenillard(250);		// Tell we are alive
	}
	else
	{
		status_led(none);
		msleep(250);
	}

	if (uart_done == 1)			// Check if reading something from the UART
	{
		uint32_t nb_bloc = uart_ptr / ENCRYPT_BLOCK_LEN;

		memcpy(&buff,&uart_buff,512);

		encrypt(buff,key,iv,nb_bloc);
		send_message(buff,nb_bloc,uart_ptr,LINKED_ADDRESS,(EVERYTYPE|FIRE_MNGMT));		// Sends a message

		memset(uart_buff,'0',UART_BUFF_LEN);
		uart_ptr = 0;
		uart_done = 0;
	}

	execJob();

	radio_stuff();

	if (check_rx == 1)			// Check if has recieved a packet
	{
		handle_rf_rx_data();
		check_rx = 0;
	}
	
	if (rx_done == 1)			// Check if has recieved a complete message (all packet recieved)
	{
		uint16_t messageLen = msgHeader.lastPacketLen + msgHeader.nbpacket * MAX_NB_BLOCK * PAYLOAD_BLOC_LEN;
		uint8_t nb_bloc = messageLen / ENCRYPT_BLOCK_LEN;

#if DEBUG > 0
		uprintf(UART0,"Mesage is %d bytes long (%d blocs):\n\r",messageLen,(nb_bloc+1));
#endif
		decrypt(rxBuff,key,iv,nb_bloc);

		fireTreatData(messageLen);

		memset(rxBuff,'0',MESSAGE_BUFF_LEN);
    	rx_done = 0;
	}
}