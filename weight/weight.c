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
volatile float doseWeight = 0;
const float BOTTLE_WEIGHT = 10; //measured in grams

/************************************************************************/
/* Header Implementation                                                */
/************************************************************************/

float Weight_CalculateMass(int atodval)
{
	float voltage = calculateVoltage(atodval);
	// 1000.0 is the conversion factor for kg to g
	// 9.81 is gravity
	// g = N * (g/kg) / m/s^2
	float max_weight = (Weight_MAXN * 1000.0) / 9.81;
	
	float weight = (voltage / AREF) * max_weight; //TODO: Add fulcrum gain
	
	return weight;
}

float Weight_DetermineDosage(float startingWeight, int numDoses)
{
	//TODO: We would want to improve it to allow a user to 
	// configure which standard bottle they are using which would determine
	// the base weight of the bottle, but we're just going to hardcode it to the one size for ease

	doseWeight = (startingWeight - BOTTLE_WEIGHT) / numDoses;
	return doseWeight;
}

Weight_LoadState Weight_CheckForLoad(int atodval)
{	
	if(atodval > MAX_ATODVAL) return Weight_LoadError;

	float voltage = calculateVoltage(atodval);

	return voltage > Weight_MINV ? Weight_LoadPresent : Weight_LoadNotPresent;
}

Weight_Change Weight_CheckForChange(int atodval)
{
	Weight_Change change;
	float newWeight = Weight_CalculateMass(atodval);
	float weightDifference = previousWeight - newWeight;
	printf("\nnewWeight: %f", newWeight);
	printf("\npreviousWeight: %f", previousWeight);
	printf("\ndoseWeight: %f", doseWeight);
	printf("\nweightDifference: %f", weightDifference);
	
	// The weight went up, track as a refill
	if(weightDifference < 0)
		change = Weight_RefillChange;
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
    float q = AREF / MAX_ATODVAL; // 10 bit atod [1024]
    float voltage = atodval * q; // min val (resting) 0.73V
	return voltage;
}