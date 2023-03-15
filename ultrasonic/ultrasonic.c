/*
 * ultrasonic.c
 * Ultrasonic Sensor(s) Used for object detection/obstacle avoidance
 * 
 * Created: 2023-03-14
 * Author: Kia Skretteberg
 */
#include "ultrasonic.h"
 
/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/
 
// Calculate the distance (in cm) of the sound pulse from the duration (in us)
float calculateDistance(long duration);
 
 
/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

volatile long echoTimeStart = 0;
volatile long echoTimeEnd = 0;
volatile Ultrasonic_Device activeDevice = Ultrasonic_None;

 
/************************************************************************/
/* Header Implementation                                                */
/************************************************************************/
 
int Ultrasonic_CheckForObstacle(Ultrasonic_Device device, float distance)
{
	float dDistance = calculateDistance(device);
	 
	return dDistance >= 0 && dDistance < distance ? 1 : 0;
}



/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/

float calculateDistance(long duration)
{
	float speed = 0.0343; // speed of sound in cm/us -- speed pulled from google as 343m/s in dry air at 20C on Feb 24th, 2023
	return (duration * speed) / 2; //calculation retrieved from datasheet (see header file)
}
