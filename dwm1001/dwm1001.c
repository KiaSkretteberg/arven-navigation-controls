/*
 * dwm1001.c
 *
 * Created: 2023-03-20
 * Author: Kia Skretteberg
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "dwm1001.h"


/************************************************************************/
/* Header Implementation                                                */
/************************************************************************/

void dwm1001_init_communication(void)
{
    // Set up our UART with the required speed.
    uart_init(DWM1001_UART_ID, DWM1001_BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(DWM1001_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(DWM1001_RX_PIN, GPIO_FUNC_UART);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(DWM1001_UART_ID, false, false);

    // Set our data format
    uart_set_format(DWM1001_UART_ID, DWM1001_DATA_BITS, DWM1001_STOP_BITS, DWM1001_PARITY);

    // TODO: Is the FIFO needed?
}

void dwm1001_request_position(void)
{
    // dwm_pos_get      see 5.3.2
    char data[] = { 0x02, 0x00 };
    uint8_t buff[20];
    uart_puts(DWM1001_UART_ID, data);
    
    uart_read_blocking(DWM1001_UART_ID, buff, 1);
}