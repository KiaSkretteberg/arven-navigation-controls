/*
 * weight.h
 * Weight (Used for presence vs not)
 * Received from atmega. Bytes XX
 *
 * Created: 2023-03-14
 * Author: Kia Skretteberg
 */

#define AREF 5.0
#define Weight_MAXN 10 // retrieved from https://www.uneotech.com/uploads/product_download/tw/Weight-10N%20ENG.pdf
#define Weight_MINV 0.01 // Experimentally determined (voltage measured when no load) TODO: determine this value for real

typedef enum
{
	Weight_NoChange,
	Weight_SmallChange,
	Weight_LargeChange
} Weight_Change;

typedef enum
{
	Weight_LoadPresent,
	Weight_LoadNotPresent
} Weight_LoadState;

// Determine the weight (in grams) that is being measured by the Weight
float Weight_CalculateMass(void);

// Check if a load is currently being measured by the Weight
Weight_LoadState Weight_CheckForLoad(void);

// Check if the weight being measured by the Weight changed compared to previous weight
// and how it changed in relation to the weight of a dose (was it roughly a dose [smallchange], or much more [largechange])
Weight_Change Weight_CheckForChange(float oldWeight, float doseWeight);