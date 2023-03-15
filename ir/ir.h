/*
 * ir.h
 * IR Sensor(s) Used for fall detection
 * Received from atmega. Bytes XX
 *
 * Created: 2023-03-14
 * Author: Kia Skretteberg
 */ 

typedef enum
{
	IR_L = 0, // left IR sensor	-- I2C addr 0x30
	IR_R = 1, // center IR sensor	-- I2C addr 0x31
	IR_None = 10
} IR_Device;

// Checks if the sensor no longer detects ground beneath it (1 if no ground ==> drop, 0 if ground ==> no drop)
int IR_CheckForDrop(void);
