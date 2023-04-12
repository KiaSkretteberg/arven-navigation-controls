/*
 * weight.c
 *
 * Created: 2023-03-14
 * Author: Kia Skretteberg
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "weight.h"

/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

float calculateVoltage(int atodval);

/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

volatile float previousWeight = 0;

/************************************************************************/
/* Header Implementation                                                */
/************************************************************************/

float Weight_CalculateMass(int atodval)
{
	float voltage = calculateVoltage(atodval);
	// 1000.0 is the conversion factor for kg to g
	// 9.81 is gravity
	// g = N * (g/kg) * m/s^2
	float max_weight = (Weight_MAXN * 1000.0 / 9.81);
	
	float weight = (voltage / AREF) * max_weight; //TODO: Add fulcrum gain
	
	return weight;
}

Weight_LoadState Weight_CheckForLoad(int atodval)
{	
	float voltage = calculateVoltage(atodval);
	return voltage > Weight_MINV ? Weight_LoadPresent : Weight_LoadNotPresent;
}

Weight_Change Weight_CheckForChange(int atodval, float doseWeight)
{
	Weight_Change change;
	float newWeight = Weight_CalculateMass(atodval);
	float weightDifference = previousWeight - newWeight;
	
	// The weight went up?? Track as no change because it's confusing
	if(weightDifference < 0)
		change = Weight_NoChange;
	// The difference is less than or equal to a dose (plus 10% for error)
	else if(weightDifference <= (doseWeight * 1.1))
		change = Weight_SmallChange;
	// Larger than a dose means something potentially bad happened
	else
		change = Weight_LargeChange;
	
	// store the new weight for later comparison
	previousWeight = newWeight;
	
	return change;
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/

float calculateVoltage(int atodval)
{
    float q = AREF / 1024.0; // 10 bit atod [1024]
    float voltage = atodval * q; // min val (resting) 0.73V
	return voltage;
}