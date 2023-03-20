/*
 * dwm1001.h
 * UWB Module for positioning
 * Utilizes UART1 (pins 6 and 7)
 *
 * Created: 2023-03-20
 * Author: Kia Skretteberg
 */

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define DWM1001_TX_PIN 4 // GPIO pin 4 [pin 6]
#define DWM1001_RX_PIN 5 // GPIO pin 5 [pin 7]

#define DWM1001_UART_ID   uart1
#define DWM1001_BAUD_RATE 115200
#define DWM1001_DATA_BITS 8
#define DWM1001_STOP_BITS 1
#define DWM1001_PARITY    UART_PARITY_NONE

void dwm1001_init_communication(void);

void dwm1001_request_position(void);