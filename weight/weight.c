/*
 * weight.c
 *
 * Created: 2023-03-14
 * Author: Kia Skretteberg
 */
#include "weight.h"

/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

/************************************************************************/
/* Header Implementation                                                */
/************************************************************************/

float Weight_DetermineWeight(void)
{
	float voltage = measureVoltage();
	// 1000.0 is the conversion factor for kg to g
	// 9.81 is gravity
	// g = N * (g/kg) * m/s^2
	float max_weight = (Weight_MAXN * 1000.0 / 9.81);
	
	float weight = (voltage / AREF) * max_weight; //TODO: Add fulcrum gain
	
	return weight;
}

Weight_LoadState Weight_CheckForLoad(void)
{
	float volt = measureVoltage();
	
	return volt > Weight_MINV ? Weight_LoadPresent : Weight_LoadNotPresent;
}

Weight_WeightChange Weight_CheckForWeightChange(float oldWeight, float doseWeight)
{
	Weight_WeightChange change;
	float newWeight = Weight_DetermineWeight();
	float weightDifference = oldWeight - newWeight;
	
	// The weight went up?? Track as no change because it's confusing
	if(weightDifference < 0)
		change = Weight_NoChange;
	// The difference is less than or equal to a dose (plus 10% for error)
	else if(weightDifference <= (doseWeight * 1.1))
		change = Weight_SmallChange;
	// Larger than a dose means something potentially bad happened
	else
		change = Weight_LargeChange;
	
	return change;
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/
