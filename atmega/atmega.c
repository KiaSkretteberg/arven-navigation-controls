/*
 * atmega.c
 *
 * Created: 2023-03-20
 *  Author: Kia Skretteberg
 */
 
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "atmega.h"
/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

struct AtmegaFrame atmega_retrieve_frame(void);

// returns the parsed struct value of the actual sensor values represented by the frame
struct SensorValues atmega_parse_frame(struct AtmegaFrame);

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
    char bytesReceived = 0;
    struct AtmegaFrame frame;
    int frame_begin = 0; // flag indicating we saw the begin of a frame
    int frame_end = 0; // flag indicating we saw the end of a frame

    while (uart_is_readable(ATMEGA_UART_ID)) 
    {
        uint8_t ch = uart_getc(ATMEGA_UART_ID);

        // start of frame seen for the first time
        if(ch == ATMEGA_START_BYTE && !frame_begin)
        {
            frame_begin = 1;
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
        }
        // data received after begin frame seen
        else if(frame_begin && bytesReceived < ATMEGA_FRAME_LENGTH) 
        {
            frames[current_frame_index] = atmega_read_byte_into_frame(frames[current_frame_index], bytesReceived, ch);
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

struct SensorValues atmega_retrieve_sensor_values(void)
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

struct SensorValues atmega_parse_frame(struct AtmegaFrame frame)
{
    struct SensorValues sv;
    char bumps = convert_string_to_hex(frame.Bumps_L_R);
    char m_directions = convert_string_to_hex(frame.Motor_Directions);

    // The bytes come in opposite order (MSB to LSB), so we need to ensure they map back properly
    sv.IR_L_Distance = convert_bytes_string_to_hex(&frame.IR_L, 1);
    sv.IR_R_Distance = convert_bytes_string_to_hex(&frame.IR_R, 1);

    sv.Ultrasonic_L_Duration = convert_bytes_string_to_hex(&frame.Ultrasonic_L, 4);
    sv.Ultrasonic_C_Duration = convert_bytes_string_to_hex(&frame.Ultrasonic_C, 4);
    sv.Ultrasonic_R_Duration = convert_bytes_string_to_hex(&frame.Ultrasonic_R, 4);

    // check the appropriate bits from the bumps value for 0/1
    sv.Bump_L = bumps & ATMEGA_BUMP_L;
    sv.Bump_R = bumps & ATMEGA_BUMP_R;

    sv.Weight = convert_byte_string_to_hex(&frame.Weight, 2);

    // check the appropriate bits from the m_directions value for 0/1
    sv.Motor_FL_Direction = m_directions & ATMEGA_MOTOR_FL_Direction;
    sv.Motor_FR_Direction = m_directions & ATMEGA_MOTOR_FR_Direction;
    // sv.Motor_ML_Direction = m_directions & ATMEGA_MOTOR_ML_Direction;
    // sv.Motor_MR_Direction = m_directions & ATMEGA_MOTOR_MR_Direction;
    // sv.Motor_BL_Direction = m_directions & ATMEGA_MOTOR_BL_Direction;
    // sv.Motor_BR_Direction = m_directions & ATMEGA_MOTOR_BR_Direction;

    sv.Motor_FL_Speed = convert_bytes_string_to_hex(&frame.Motor_Speed_FL, 1);
    sv.Motor_FR_Speed = convert_bytes_string_to_hex(&frame.Motor_Speed_FR, 1);
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
        *frame.IR_L += c;
    } else if (byteCount < 4) {
        *frame.IR_R += c;
    } else if (byteCount < 9) {
        *frame.Ultrasonic_L += c;
    } else if (byteCount < 14) {
        *frame.Ultrasonic_C += c;
    } else if (byteCount < 19) {
        *frame.Ultrasonic_R += c;
    } else if (byteCount < 20) {
        frame.Bumps_L_R = c;
    } else if (byteCount < 23) {
        *frame.Weight += c;
    } else if (byteCount < 24) {
        frame.Battery = c;
    } else if (byteCount < 25) {
        frame.Motor_Directions = c;
    } else if (byteCount < 27) {
        *frame.Motor_Speed_FL += c;
    } else if (byteCount < 29) {
        *frame.Motor_Speed_FR += c;
    // } else if (byteCount < 31) {
    //     *frame.Motor_Speed_ML += c;
    // } else if (byteCount < 33) {
    //     *frame.Motor_Speed_MR += c;
    // } else if (byteCount < 35) {
    //     *frame.Motor_Speed_BL += c;
    // } else if (byteCount < 37) {
    //     *frame.Motor_Speed_BR += c;
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
        // keep track of the number of bytes we've processed, until we have no bytes left
        --byteIndex;
    }
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