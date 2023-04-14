/*
 * weight.h
 * Weight (Used for presence vs not)
 * Received from atmega. Bytes XX
 *
 * Created: 2023-03-14
 * Author: Kia Skretteberg
 */

#define AREF 5.0
#define MAX_ATODVAL 1024.0
#define Weight_MAXN 10 // retrieved from https://www.uneotech.com/uploads/product_download/tw/Weight-10N%20ENG.pdf
#define Weight_MINV 0.00 // Experimentally determined (voltage measured when no load) TODO: determine this value for real


typedef enum
{
	Weight_NoChange,
	Weight_SmallChange,
	Weight_LargeChange
} Weight_Change;

typedef enum
{
	Weight_LoadPresent,
	Weight_LoadNotPresent,
	Weight_LoadError
} Weight_LoadState;

// Determine the weight (in grams) that the atodval represents
float Weight_CalculateMass(int atodval);

// Check if a load is currently being indicated by the atodval measured
Weight_LoadState Weight_CheckForLoad(int atodval);

// Check if the atodval indicates a change in weight compared to previous weight
// and how it changed in relation to the weight of a dose (was it roughly a dose [smallchange], or much more [largechange])
Weight_Change Weight_CheckForChange(int atodval, float doseWeight);