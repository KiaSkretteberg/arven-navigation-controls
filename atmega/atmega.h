/* Communication with the Atmega328P via UART0 (pins 1 and 2)
    Utilizing Frames of the following structure:
    $FFFFF1FFFF1FFFF1FFFFA3FF1^ 

    The above is broken up into 9 segments varying in the number of bytes that represent them.

    Segment 1: (1 byte)
    Indication of which of the 8 sensors have changed.
    The values that are indicated as being modified are the next 7 segments (segment 9 is excluded since it is just a battery indicator)

   * ---------------------------------------------------------------------------------------------
   * |    b7    |    b6    |     b5    |    b4    |     b3     |   b2    |    b1    |     b0     |
   * ---------------------------------------------------------------------------------------------
   * |  IR Left | IR Right |  US Left  |  US Ctr  |  US Right  |  Bumps  |  Weight  |  Encoders  |
   * ---------------------------------------------------------------------------------------------

    Segment 2: (2 bytes) 
    Left IR Sensor
    Values 00-FF, representing a number of mm in hex

    Segment 3: (2 bytes)
    Right IR Sensor
    Values 00-FF, representing a number of mm in hex

    Segment 4: (5 bytes)
    Left Ultrasonic Sensor
    Values 00000-1FFFF
    A measure of counts (in 0.5us?) that it took the echo to send and receive (needs to be / 2)

    Segment 5: (5 bytes)
    Center Ultrasonic Sensor
    Values 00000-1FFFF
    A measure of counts (in 0.5us?) that it took the echo to send and receive (needs to be / 2)

    Segment 6: (5 bytes)
    Right Ultrasonic Sensor
    Values 00000-1FFFF
    A measure of counts (in 0.5us?) that it took the echo to send and receive (needs to be / 2)

    Segment 7: (1 byte)
    Left and Right Bump Sensor
    A hex value representing 2 bits with b0 being the right sensor, and b1 being the left sensor.
    1 means there is an obstacle. 0 means there is no obstacle.

   * ------------------------------------------
   * |    Hex Value    |    b1    |     b0    |
   * ------------------------------------------
   * |        A        |    1     |     0     | 
   * ------------------------------------------
   * |        B        |    1     |     1     | 
   * ------------------------------------------
   * |        C        |    0     |     0     | 
   * ------------------------------------------
   * |        D        |    0     |     1     | 
   * ------------------------------------------
   The values are chosen as the bit values represent the last 2 bits of the hex values (A = 1010 ==> 10, B = 1011 ==> 11, etc)

    Segment 8: (3 bytes)
    Weight (force sensing resistor)
    A raw AtoD value, from a 10 bit ADC, representing the voltage measured between 0 and 5V, where 5V = 10N (max force)

    Segment 9: (1 or 3 bytes)
    Battery level (if 1 byte, is 0/1; if 3 bytes, is an AtoD value similar to segment 8)

    Segment 10: (??)
    Encoders
    ??
*/

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define ATMEGA_TX_PIN 0 // GPIO pin 0 [pin 1]
#define ATMEGA_RX_PIN 1 // GPIO pin 1 [pin 2]

#define ATMEGA_UART_ID   uart0
#define ATMEGA_BAUD_RATE 115200
#define ATMEGA_DATA_BITS 8
#define ATMEGA_STOP_BITS 1
#define ATMEGA_PARITY    UART_PARITY_NONE

#define ATMEGA_MAX_FRAMES_STORED 5   // max number of frames that can be stored before we start overwriting the oldest ones
#define ATMEGA_FRAME_LENGTH      25  // not inclusive of start/end bytes
#define ATMEGA_START_BYTE        '$' // indicator of a start frame
#define ATMEGA_END_BYTE          '^' // indicator of an end frame

void atmega_init_communication(void);
void atmega_receive_data();
void atmega_send_data();

