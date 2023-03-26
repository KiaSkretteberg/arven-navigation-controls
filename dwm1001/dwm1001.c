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

int read_value(uint8_t expect_type, uint8_t * buff);


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
    uint8_t buff[20];
    uint8_t error;
    // dwm_loc_get      see 5.3.10
    uart_putc_raw(DWM1001_UART_ID, 0x0C);
    uart_putc_raw(DWM1001_UART_ID, 0x00);

    //check if there's an error in the command
    error = read_value(0x40, buff);
    // get the device's position
    error = read_value(0x41, buff);
    // get the position to the anchors (can help in getting home... if home isn't 0,0)
    error = read_value(0x49, buff);
}

int read_value(uint8_t expect_type, uint8_t * buff)
{
    unsigned char count = 0;
    // max number of bytes to be read, at most is 253
    unsigned char maxLength = 253;
    
    // while there's data to read and we haven't read the end of this value (read all bytes indicated in value)
    while(uart_is_readable(DWM1001_UART_ID) && count < maxLength + 2)
    {
        uint8_t c = uart_getc(DWM1001_UART_ID);
        ++count;
        if(count == 1)
        {
            // if the type doesn't match the expected type, throw an error!
            if(c != expect_type)
                return -1;
        }
        // grab the length of how many bytes to receive next
        else if(count == 2)
        {
            maxLength = c;
        }
        else
        {
            *buff = c;
            ++buff;
        }
    }

    return 0;
}