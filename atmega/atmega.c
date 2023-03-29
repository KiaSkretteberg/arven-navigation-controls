/*
 * atmega.c
 *
 * Created: 2023-03-20
 *  Author: Kia Skretteberg
 */
 
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "atmega.h"
/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

const uint TEST_LED_PIN = 16;
const uint TEST_LED_PIN2 = 17;

// the buffer that holds the bytes received during a single frame read
volatile char rxBuff[ATMEGA_FRAME_LENGTH + 1];
// count of bytes received as part of a single frame, reset with each frame start seen
volatile char bytesReceived = 0;
// flag indicating we saw the begin of a frame
volatile char frame_begin = 0; 

struct AtmegaFrame atmega_retrieve_frame(void);

// returns the parsed struct value of the actual sensor values represented by the frame
struct AtmegaSensorValues atmega_parse_frame(struct AtmegaFrame);

// modifies the frame to have the up to date mapping of bytes and returns it
struct AtmegaFrame atmega_read_byte_into_frame(struct AtmegaFrame frame, char byteCount, char c);

// converts the passed in byte array (string) to the actual hex values they represent
// e.g. if bytes = ['a','f','2']
// the returned value will be 0xAF2
long convert_bytes_string_to_hex(char * bytes, char startByteIndex);

// converts a single string character to it's hex value that it represents (0-F)
// returns -1 if the value can't be converted to 0-F
// solution adapted from https://stackoverflow.com/questions/33982870/how-to-convert-char-array-to-hexadecimal
char convert_string_to_hex(char c);

// Parses the rxBuff into an AtmegaFrame
void atmega_parse_bytes(void);

 
/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

volatile struct AtmegaFrame frames[ATMEGA_MAX_FRAMES_STORED];
volatile int current_frame_index = 0;

/************************************************************************/
/* Header Implementation                                                */
/************************************************************************/

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
    if (uart_is_readable(ATMEGA_UART_ID)) 
    {
        char ch = uart_getc(ATMEGA_UART_ID);

        // start of frame seen for the first time
        if(ch == ATMEGA_START_BYTE)
        {
            struct AtmegaFrame frame;
            // reset the buffer to be empty
            strcpy(rxBuff, "");
            // reset byte count
            bytesReceived = 0;
            // start frame seen
            frame_begin = 1;
            // create a new frame
            frames[current_frame_index] = frame;
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
            // read all the bytes into the frame
            atmega_parse_bytes();
        }
        // data received after begin frame seen
        else if(frame_begin && bytesReceived < ATMEGA_FRAME_LENGTH) 
        {
            // read the byte into the buffer to be parsed later
            rxBuff[bytesReceived] = ch;
            // keep track of how many bytes have been received so we'll know when we reach the end of frame
            ++bytesReceived;
            // ++*rxBuff;
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

struct AtmegaSensorValues atmega_retrieve_sensor_values(void)
{
    return atmega_parse_frame(atmega_retrieve_frame());
}

struct AtmegaFrame atmega_retrieve_frame(void)
{
    return frames[current_frame_index];
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/

void atmega_parse_bytes(void)
{
    char bytesRead = 0;
    char buff[ATMEGA_FRAME_LENGTH + 1];
    strcpy(buff, rxBuff);
    atmega_send_data("\nPARSING...\n");
    atmega_send_data(buff);

    while(*buff)
    {
        frames[current_frame_index] = atmega_read_byte_into_frame(frames[current_frame_index], bytesRead, *buff);
        ++bytesRead;
        ++*buff;
    }
}

struct AtmegaSensorValues atmega_parse_frame(struct AtmegaFrame frame)
{
    struct AtmegaSensorValues sv;
    char changed = convert_bytes_string_to_hex(frame.Changed, 1);
    char bumps = convert_string_to_hex(frame.Bumps_L_R);
    char m_directions = convert_bytes_string_to_hex(frame.Motor_Directions, 1);

    // Check each bit of the changed byte to see which bytes have changes
    sv.Changes              = changed > 0;
    sv.IR_L_Changed         = changed & ATMEGA_IR_L_CHANGED;
    sv.IR_R_Changed         = changed & ATMEGA_IR_R_CHANGED;
    sv.Ultrasonic_L_Changed = changed & ATMEGA_ULTRASONIC_L_CHANGED;
    sv.Ultrasonic_C_Changed = changed & ATMEGA_ULTRASONIC_C_CHANGED;
    sv.Ultrasonic_R_Changed = changed & ATMEGA_ULTRASONIC_R_CHANGED;
    sv.Bumps_Changed        = changed & ATMEGA_BUMPS_CHANGED;
    sv.Weight_Changed       = changed & ATMEGA_WEIGHT_CHANGED;
    sv.Encoders_Changed     = changed & ATMEGA_ENCODERS_CHANGED;

    // The bytes come in opposite order (MSB to LSB), so we need to ensure they map back properly
    sv.IR_L_Distance = convert_bytes_string_to_hex(frame.IR_L, 1);
    sv.IR_R_Distance = convert_bytes_string_to_hex(frame.IR_R, 1);

    sv.Ultrasonic_L_Duration = convert_bytes_string_to_hex(frame.Ultrasonic_L, 4);
    sv.Ultrasonic_C_Duration = convert_bytes_string_to_hex(frame.Ultrasonic_C, 4);
    sv.Ultrasonic_R_Duration = convert_bytes_string_to_hex(frame.Ultrasonic_R, 4);

    // check the appropriate bits from the bumps value for 0/1
    sv.Bump_L = bumps & ATMEGA_BUMP_L;
    sv.Bump_R = bumps & ATMEGA_BUMP_R;

    sv.Weight = convert_bytes_string_to_hex(frame.Weight, 2);

    // check the appropriate bits from the m_directions value for 0/1
    sv.Motor_FL_Direction = m_directions & ATMEGA_MOTOR_FL_Direction;
    sv.Motor_FR_Direction = m_directions & ATMEGA_MOTOR_FR_Direction;
    // sv.Motor_ML_Direction = m_directions & ATMEGA_MOTOR_ML_Direction;
    // sv.Motor_MR_Direction = m_directions & ATMEGA_MOTOR_MR_Direction;
    // sv.Motor_BL_Direction = m_directions & ATMEGA_MOTOR_BL_Direction;
    // sv.Motor_BR_Direction = m_directions & ATMEGA_MOTOR_BR_Direction;

    sv.Motor_FL_Speed = convert_bytes_string_to_hex(frame.Motor_Speed_FL, 1);
    sv.Motor_FR_Speed = convert_bytes_string_to_hex(frame.Motor_Speed_FR, 1);
    // sv.Motor_ML_Speed = convert_bytes_string_to_hex(&frame.Motor_Speed_ML, 1);
    // sv.Motor_MR_Speed = convert_bytes_string_to_hex(&frame.Motor_Speed_MR, 1);
    // sv.Motor_BL_Speed = convert_bytes_string_to_hex(&frame.Motor_Speed_BL, 1);
    // sv.Motor_BR_Speed = convert_bytes_string_to_hex(&frame.Motor_Speed_BR, 1);

    // check to see if the converted value is 0 or 1
    sv.Battery_Low = convert_string_to_hex(frame.Battery) & 1;

    return sv;
}

struct AtmegaFrame atmega_read_byte_into_frame(struct AtmegaFrame frame, char byteCount, char c) 
{
    if(byteCount < 2) {
        *frame.Changed = c;
        ++*frame.Changed;
    } else if(byteCount < 4) {
        *frame.IR_L = c;
        ++*frame.IR_L;
    } else if (byteCount < 6) {
        *frame.IR_R = c;
        ++*frame.IR_R;
    } else if (byteCount < 11) {
        *frame.Ultrasonic_L = c;
        ++*frame.Ultrasonic_L;
    } else if (byteCount < 16) {
        *frame.Ultrasonic_C = c;
        ++*frame.Ultrasonic_C;
    } else if (byteCount < 21) {
        *frame.Ultrasonic_R = c;
        ++*frame.Ultrasonic_R;
    } else if (byteCount < 22) {
        frame.Bumps_L_R = c;
    } else if (byteCount < 25) {
        *frame.Weight = c;
        ++*frame.Weight;
    } else if (byteCount < 26) {
        frame.Battery = c;
    } else if (byteCount < 28) {
        *frame.Motor_Directions = c;
        ++*frame.Motor_Directions;
    } else if (byteCount < 30) {
        *frame.Motor_Speed_FL = c;
        ++*frame.Motor_Speed_FL;
    } else if (byteCount < 32) {
        *frame.Motor_Speed_FR = c;
        ++*frame.Motor_Speed_FR;
    // } else if (byteCount < 34) {
    //     *frame.Motor_Speed_ML = c;
    //    ++*frame.Motor_Speed_ML;
    // } else if (byteCount < 36) {
    //     *frame.Motor_Speed_MR = c;
    //    ++*frame.Motor_Speed_MR;
    // } else if (byteCount < 38) {
    //     *frame.Motor_Speed_BL = c;
    //    ++*frame.Motor_Speed_BL;
    // } else if (byteCount < 40) {
    //     *frame.Motor_Speed_BR = c;
    //    ++*frame.Motor_Speed_BR;
    }

    return frame;
}

long convert_bytes_string_to_hex(char * bytes, char startByteIndex)
{
    char byteIndex = startByteIndex;
    long value;
    while(byteIndex >= 0)
    {
        // starting from the MSB, add the current byte value to the return value
        // with the approrpiate position in significance
        value = *bytes * (byteIndex > 0 ? (byteIndex * 16) : 1);
        // move to the next byte
        ++*bytes;

        // break out of the loop once we've finished processing the last one since we can't go below 0
        if(byteIndex == 0)
            break;

        // keep track of the number of bytes we've processed, until we have no bytes left
        --byteIndex;
    }
    return value;
}

char convert_string_to_hex(char c)
{
    // ensure the character is lowercase for easier checking 
    c = tolower(c);

    if( c >= '0' && c <= '9' ) {
        return c - '0'; //convert to the actual hex value between 0 and 9
    }
    if( c >= 'a' && c <= 'f' ) {
        return c - 'a' + 10; // convert to the actual hex value between a and f
    }

    return -1;
}