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
	IR_L = 0, // segment 2 (2 bytes)
	IR_R = 1, // segment 3 (2 bytes)
} IR_Device;

// Checks if the sensor no longer detects ground beneath it (1 if no ground ==> drop, 0 if ground ==> no drop)
int IR_CheckForDrop(void);
