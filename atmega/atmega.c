#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "atmega.h"

volatile char frames[ATMEGA_MAX_FRAMES_STORED][ATMEGA_FRAME_LENGTH + 1] = {""};
volatile int current_frame_index = 0;

void atmega_init_communication(void)
{
    // Set up our UART with the required speed.
    uart_init(ATMEGA_UART_ID, ATMEGA_BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(ATMEGA_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(ATMEGA_RX_PIN, GPIO_FUNC_UART);

     // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(ATMEGA_UART_ID, false, false);

    // Set our data format
    uart_set_format(ATMEGA_UART_ID, ATMEGA_DATA_BITS, ATMEGA_STOP_BITS, ATMEGA_PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(ATMEGA_UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART0_IRQ, atmega_receive_data);
    irq_set_enabled(UART0_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(ATMEGA_UART_ID, true, false);
}

void atmega_receive_data(void)
{
    char bytesReceived = 0;
    char frame[26] = "";
    int frame_begin = 0; // flag indicating we saw the begin of a frame
    int frame_end = 0; // flag indicating we saw the end of a frame

    while (uart_is_readable(ATMEGA_UART_ID)) 
    {
        uint8_t ch = uart_getc(ATMEGA_UART_ID);

        // start of frame seen for the first time
        if(ch == ATMEGA_START_BYTE && !frame_begin)
        {
            frame_begin = 1;
        }
        // end frame seen after a start frame is seen and we got the expected number of bytes
        else if(ch == ATMEGA_END_BYTE && frame_begin && bytesReceived == ATMEGA_FRAME_LENGTH)
        {
            // move storage of frames to the next slot available
            ++current_frame_index;
            // roll over if we reached the end of the total amount of frames allowed (overwrite oldest frame)
            if(current_frame_index == ATMEGA_MAX_FRAMES_STORED - 1) 
                current_frame_index = 0;
            // reset frame begin so we know that we are no longer reading a frame
            frame_begin = 0;
        }
        // data received after begin frame seen
        else if(frame_begin && bytesReceived < ATMEGA_FRAME_LENGTH) 
        {
            *frames[current_frame_index] += ch;
            ++bytesReceived;
        }
        // some sort of error (frame_begin not seen, frame_end seen too early, frame_end seen before frame_begin)
        else
        {
            //TODO: What to do if there's a frame error?
            // Re-request data? Ignore and move on? Record error somewhere?
        }
    }
}

void atmega_send_data(char * data)
{
    uart_puts(ATMEGA_UART_ID, data);
}