#include "lib/stdio.h"

#define UART_BUFF_LEN 512

extern volatile uint8_t uart_done;
extern uint8_t uart_buff[];
extern uint16_t uart_ptr;

void handle_uart_cmd(uint8_t c);