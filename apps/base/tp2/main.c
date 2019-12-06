/****************************************************************************
 *   apps/base/i2c_temp/main.c
 *
 * TMP101 I2C temperature sensor example
 *
 * Copyright 2013-2014 Nathael Pajani <nathael.pajani@ed3l.fr>
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *************************************************************************** */


#include "core/system.h"
#include "core/systick.h"
#include "core/pio.h"
#include "lib/stdio.h"
#include "drivers/i2c.h"
#include "drivers/serial.h"
#include "drivers/gpio.h"
#include "extdrv/status_led.h"
#include "extdrv/tmp101_temp_sensor.h"


#define MODULE_VERSION    0x04
#define MODULE_NAME "GPIO Demo Module"


#define SELECTED_FREQ  FREQ_SEL_48MHz

/***************************************************************************** */
/* Pins configuration */
/* pins blocks are passed to set_pins() for pins configuration.
 * Unused pin blocks can be removed safely with the corresponding set_pins() call
 * All pins blocks may be safelly merged in a single block for single set_pins() call..
 */
const struct pio_config common_pins[] = {
	/* UART 0 */
	{ LPC_UART0_RX_PIO_0_1,  LPC_IO_DIGITAL },
	{ LPC_UART0_TX_PIO_0_2,  LPC_IO_DIGITAL },
	/* I2C 0 */
	{ LPC_I2C0_SCL_PIO_0_10, (LPC_IO_DIGITAL | LPC_IO_OPEN_DRAIN_ENABLE) },
	{ LPC_I2C0_SDA_PIO_0_11, (LPC_IO_DIGITAL | LPC_IO_OPEN_DRAIN_ENABLE) },
	ARRAY_LAST_PIO,
};

#define TMP101_ADDR  0x94  /* Pin Addr0 (pin5 of tmp101) connected to VCC */
struct tmp101_sensor_config tmp101_sensor = {
	.bus_num = I2C0,
	.addr = TMP101_ADDR,
	.resolution = TMP_RES_ELEVEN_BITS,
};

const struct pio temp_alert = LPC_GPIO_0_7;

const struct pio status_led_green = LPC_GPIO_0_0;
const struct pio status_led_orange = LPC_GPIO_0_10;
const struct pio status_led_red = LPC_GPIO_0_11;

struct pio orange_led = LPC_GPIO_1_4;

/***************************************************************************** */
void system_init()
{
	/* Stop the watchdog */
	startup_watchdog_disable(); /* Do it right now, before it gets a chance to break in */
	system_brown_out_detection_config(0); /* No ADC used */
	system_set_default_power_state();
	clock_config(SELECTED_FREQ);
	set_pins(common_pins);
	gpio_on();
	status_led_config(&status_led_green, &status_led_red);
	uint32_t mode = LPC_IO_MODE_PULL_UP | LPC_IO_DRIVE_HIGHCURENT;
	/* We must have GPIO on for status led. Calling it many times in only a waste
	 *  of time, no other side effects */
	/* Status Led GPIO. Turn Green on. */
	config_gpio(&status_led_orange, mode, GPIO_DIR_OUT, 1);
	/* System tick timer MUST be configured and running in order to use the sleeping
	 * functions */
	systick_timer_on(1); /* 1ms */
	systick_start();
}

/* Define our fault handler. This one is not mandatory, the dummy fault handler
 * will be used when it's not overridden here.
 * Note : The default one does a simple infinite loop. If the watchdog is deactivated
 * the system will hang.
 * An alternative would be to perform soft reset of the micro-controller.
 */
void fault_info(const char* name, uint32_t len)
{
	uprintf(UART0, name);
	while (1);
}



/***************************************************************************** */
int main(void)
{
	system_init();
	uart_on(UART0, 115200, NULL);

	i2c_on(I2C0, I2C_CLK_100KHz, I2C_MASTER);

	uprintf(UART0, "Ici\n");

	/* Configure onboard temp sensor */
	//temp_config(UART0);

	gpio_clear(status_led_red);
	gpio_clear(status_led_orange);
	gpio_clear(status_led_green);
	while (1) {
		//chenillard(250);
		gpio_clear(status_led_orange);
		gpio_set(status_led_red);
		msleep(5000);

		gpio_set(status_led_orange);
		msleep(2000);

		gpio_clear(status_led_red);
		gpio_clear(status_led_orange);
		gpio_set(status_led_green);
		msleep(5000);

		gpio_clear(status_led_green);
		gpio_set(status_led_orange);
		msleep(2000);
	}
	return 0;
}

