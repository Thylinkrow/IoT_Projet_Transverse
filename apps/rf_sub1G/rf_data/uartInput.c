#include "uartInput.h"

volatile uint8_t uart_done = 0;
uint8_t uart_buff[UART_BUFF_LEN];
uint16_t uart_ptr = 0;

// Reads from the UART
void handle_uart_cmd(uint8_t c)
{
	if (uart_ptr < UART_BUFF_LEN)
	{
		// When reading a caracter puts it a placeholder char array
		uart_buff[uart_ptr++] = c;
	}
	if ((c == '\n') || (c == '\r'))
	{
		// Tells the uart is done
		uart_done = 1;
	}
}