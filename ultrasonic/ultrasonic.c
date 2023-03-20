/*
 * ultrasonic.c
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

 
/************************************************************************/
/* Header Implementation                                                */
/************************************************************************/
 
int Ultrasonic_CheckForObstacle(long duration)
{
	float dDistance = calculateDistance(duration);
	 
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
