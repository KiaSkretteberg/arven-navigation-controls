/*
 * ir.c
 *
 * Created: 2023-03-14
 * Author: Kia Skretteberg
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "ir.h"

/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

/************************************************************************/
/* Header Implementation                                                */
/************************************************************************/


// Checks if the sensor no longer detects ground beneath it (1 if no ground ==> drop, 0 if ground ==> no drop)
bool IR_CheckForDrop(char distance, char expectedDistance)
{
	return distance > expectedDistance;
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/
