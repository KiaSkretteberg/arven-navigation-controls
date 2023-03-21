/*
 * ultrasonic.h
 * Ultrasonic Sensor(s) Used for object detection/obstacle avoidance
 * Received from atmega. Bytes XX
 * 
 * Created: 2023-03-14
 * Author: Kia Skretteberg
 */ 

typedef enum
{
	Ultrasonic_L = 0, // segment 4 (5 bytes)
	Ultrasonic_C = 1, // segment 5 (5 bytes)
	Ultrasonic_R = 2, // segment 6 (5 bytes)
} Ultrasonic_Device;

// Determine if there's an obstacle within a specified range based on the duration
// 0 = no obstacle, 1 = obstacle
bool Ultrasonic_CheckForObstacle(long duration, int range);
