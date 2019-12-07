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

#define DEBUG 1

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

uint8_t test[PAYLOAD_BLOC_LEN*5] = {'a','1','c','e','c','e','c','e','c','e','c','e','1','a','\n','\r',
									'b','2','c','e','c','e','c','e','c','e','c','e','2','b','\n','\r',
									'c','3','c','e','c','e','c','e','c','e','c','e','3','c','\n','\r',
									'd','4','c','e','c','e','c','e','c','e','c','e','4','d','\n','\r',
									'e','5','c','e','c','e','c','e','c','\n','\r'};

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
#ifdef DEBUG
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

#ifdef DEBUG	
	uprintf(UART0, "App started\n\r");
	uprintf(UART0, "\t- Device address: %x\n\r\t- Linked address: %x\n\r",DEVICE_ADDRESS,LINKED_ADDRESS);
#endif

	memset(uart_buff,'0',UART_BUFF_LEN);
	memset(readBuff,'0',MESSAGE_BUFF_LEN);

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
		encrypt(uart_buff,key,nb_bloc);
		send_on_rf(uart_buff,nb_bloc,uart_ptr,LINKED_ADDRESS,UPLINK);		// Sends a message

		memset(uart_buff,'0',UART_BUFF_LEN);
		uart_ptr = 0;
		uart_done = 0;
	}

	radio_stuff();

	if (check_rx == 1)			// Check if has recieved a packet
	{
		handle_rf_rx_data();
		check_rx = 0;
	}
	
	if (rx_done == 1)			// Check if has recieved a complete message (all packet recieved)
	{
		uint16_t messageLen = messageInfo.lastPacketLen + messageInfo.nbpacket * MAX_NB_BLOCK * PAYLOAD_BLOC_LEN;
		uint8_t nb_bloc = messageLen / ENCRYPT_BLOCK_LEN;

		uprintf(UART0,"Mesage is %d bytes long (%d blocs):\n\r",messageLen,(nb_bloc+1));
		decrypt(readBuff,key,nb_bloc);

		fireTreatData(messageLen);

		memset(readBuff,'0',MESSAGE_BUFF_LEN);
    	rx_done = 0;
	}
}