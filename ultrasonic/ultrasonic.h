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
	Ultrasonic_L = 0, // left HC-SR04 sensor	-- PD5/PD6
	Ultrasonic_C = 1, // center HC-SR04 sensor	-- PD7/PB0
	Ultrasonic_R = 2, // right HC-SR04 sensor	-- PB1/PB2
	//Ultrasonic_All = 3
	Ultrasonic_None = 10
} Ultrasonic_Device;

// Determine if there's an obstacle within a specified distance, of the specified device, 
// 0 = no obstacle, 1 = obstacle
int Ultrasonic_CheckForObstacle(Ultrasonic_Device device, float distance);

// Get the current distance from the specified device, in cm, for a potential obstacle
// if distance is greater than 400 (4m), no obstacle could be detected
float Ultrasonic_GetEchoDistance(Ultrasonic_Device device);
