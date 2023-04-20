#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "motors.h"
#include "dwm1001.h"
#include "atmega.h"
#include "weight.h"
#include "ultrasonic.h"
#include "ir.h"
#include "web.h"

/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

const char WIFI_NETWORK_NAME[] = "PH1";
const char WIFI_PASSWORD[] = "12345678";

// monitor current state of motor so instructions are only sent for changes
volatile MotorDirection currentMotorDirection_L = Motor_Stopped;
volatile MotorDirection currentMotorDirection_R = Motor_Stopped;

const int SPEED = 40;
const int TURN_SPEED = 50;

/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

void find_user(struct AtmegaSensorValues sensorValues);
void turn_right();
void turn_left();
void go_forward();
void go_backward();
void stop();

int main() {
    int scheduleId = -1; //may be populated during operation if a schedule is operating
    struct AtmegaSensorValues sensorValues;
    // keep track of previous state for the weight sensor so we can detect when it changes
    Weight_LoadState previousLoadState = Weight_LoadNotPresent;
    // track the starting weight and number of doses available for use in calculating per-dose weight
    float startingWeight = 0;
    int numDoses = 0;
    
    stdio_init_all();

    dwm1001_init_communication();
    atmega_init_communication();
    motor_init_all();

    // wait 2seconds to allow debugging connection
    sleep_ms(2000);

    web_init(WIFI_NETWORK_NAME, WIFI_PASSWORD, "Arven", NULL, NULL, NULL);

    while (true) 
    {
        // if we don't have a schedule, check for a schedule
        if(scheduleId == -1)
        {
            web_request_check_schedule();
            // capture the schedule id once the web request has finished
            scheduleId = web_response_check_schedule();
        }
        // we have a schedule, we need to find the user and check dosage stats
        else
        {
            // get the frame info from the atmega
            // TODO: We should toggle control of the atmega code detecting the sensors based on if we want data
            sensorValues = atmega_retrieve_sensor_values();

            if(numDoses <= 0) 
            {
                // capture the new dose amount once the web request has finished
                numDoses = web_response_retrieve_dose_stats();

                // calculate the new per-dose weight to be used when calculating the weight change
                if(numDoses > 0 && startingWeight > 0)
                {
                    float doseWeight = Weight_DetermineDosage(startingWeight, numDoses);
                    printf("\ndoseWeight: %f", doseWeight);
                }
            }

            // if we don't have a starting weight, log the current weight as the starting weight
            // then determine how much a single dose should weigh based on amount of doses remaining
            if(!startingWeight) 
            {
                startingWeight = Weight_CalculateMass(sensorValues.Weight);
                printf("\nstartingWeight: %f", startingWeight);
                web_request_retrieve_dose_stats(scheduleId);
            }
            
            // if there were changes in the sensor values, react to them
            //TODO: This should detect specific changes but right now it's hardcoded and essentially ignored
            if(sensorValues.Changes) 
            {        
                // Check if we currently have something on the weight sensor
                Weight_LoadState newLoadState = Weight_CheckForLoad(sensorValues.Weight); 

                // if the state changed from something removed, to something added, check to see 
                // how much the weight changed from the last time there was weight
                if(newLoadState == Weight_LoadPresent && newLoadState != previousLoadState) 
                {
                    // check how big the change was
                    Weight_Change change = Weight_CheckForChange(sensorValues.Weight);

                    switch(change)
                    {
                        // the change increased the weight so the medication was refilled
                        case Weight_RefillChange:
                            printf("\nRefill triggered");
                            // determine the new starting weight
                            startingWeight = Weight_CalculateMass(sensorValues.Weight);
                            numDoses = 0;
                            //TODO: The api requests needs to log a refill somehow
                            web_request_retrieve_dose_stats(scheduleId);
                            break;
                        // the change was too big and may indicate an overdose, but we'll still log a delivery
                        case Weight_LargeChange:
                            printf("\nlarge change detected");
                            // TODO: log an event because this change was a possible concern
                        // the change was normal, just log a delivery
                        case Weight_SmallChange:
                            printf("\nnormal dose detected");
                            web_request_log_delivery(scheduleId);
                            break;
                        // any other change doesn't matter and may have been a hiccup
                        default:
                            break;
                    }
                }
                // update the load state for later comparison (only if it's not an error)
                if(newLoadState != Weight_LoadError) previousLoadState = newLoadState;

                //find_user(sensorValues);
            }
        }
    }
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/

void find_user(struct AtmegaSensorValues sensorValues)
{
    // check if there is an obstacle within 30cm
    bool obstacleLeft = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_L_Duration, 30);
    bool obstacleCentre = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_C_Duration, 30);
    bool obstacleRight = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_R_Duration, 30);

    // Check if the ground (60mm -- 6cm) is still there
    bool dropImminentLeft = IR_CheckForDrop(sensorValues.IR_L_Distance, 60); 
    bool dropImminentRight = IR_CheckForDrop(sensorValues.IR_R_Distance, 60);

    // Check if any of the backup sensors are being pressed
    bool obstacleRear = sensorValues.Bump_L || sensorValues.Bump_R;

    //struct DWM1001_Position position = dwm1001_request_position();
    // TODO: retrieve the user location into somewhere in order to run comparisons against our position
    //web_request("/get_user_location");
    //TODO: We need a function to parse out the user's location into DWM1001_Position

    // drop imminent
    if(dropImminentLeft || dropImminentRight)
    {
        printf("\ndrop");
        //just stop if we're about to fall
        stop();
        //TODO: We may want to backup/turn if only one has a drop pending  
        //TODO: If we sit here too long, report a failed delivery         
    }
    // no drop, proceed to check other sensors
    else
    {
        // no obstacles in front, so go forward
        if(!obstacleCentre && !obstacleLeft && !obstacleRight) 
        {
            printf("\nno obstacle");
            go_forward(); 
        } 
        // obstacle directly in front, and to the left, so turn right
        else if(obstacleCentre && obstacleLeft)
        {
            turn_right();
        }
        // obstacle directly in front, and to the right, so turn left
        else if (obstacleCentre && obstacleRight)
        {
            turn_left();
        }
        // just an obstacle in front, so turn towards the destination
        else if (obstacleCentre && !obstacleRight && !obstacleLeft)
        {
            //TODO: pick a direction that takes us closer to the destination
            turn_right();
        }
        // obstacles all in front, but no rear, so backup
        else if (obstacleCentre && obstacleRight && obstacleLeft && !obstacleRear)
        {
            go_backward();
        }
        // obstacles everywhere, just stop
        else
        {
            stop();
            //TODO: If we sit here too long, report a failed delivery
        }
    } 
}


void turn_right()
{
    if(currentMotorDirection_R != Motor_Reverse)
    {
        printf("\ngo backward right");
        currentMotorDirection_R = Motor_Reverse;
        motor_reverse(Motor_FR, SPEED);
    }
    if(currentMotorDirection_L != Motor_Forward)
    {
        printf("\ngo forward left");
        currentMotorDirection_L = Motor_Forward;
        motor_forward(Motor_FL, SPEED); 
    }
}

void turn_left()
{
    if(currentMotorDirection_R != Motor_Forward)
    {
        printf("\ngo forward right");
        currentMotorDirection_R = Motor_Forward;
        motor_forward(Motor_FR, SPEED);
    }
    if(currentMotorDirection_L != Motor_Reverse)
    {
        printf("\ngo backward left");
        currentMotorDirection_L = Motor_Reverse;
        motor_reverse(Motor_FL, SPEED); 
    }
}

void go_forward()
{
    if(currentMotorDirection_R != Motor_Forward)
    {
        printf("\ngo forward right");
        currentMotorDirection_R = Motor_Forward;
        motor_forward(Motor_FR, SPEED);
    }
    if(currentMotorDirection_L != Motor_Forward)
    {
        printf("\ngo forward left");
        currentMotorDirection_L = Motor_Forward;
        motor_forward(Motor_FL, SPEED); 
    }
}

void go_backward()
{
    if(currentMotorDirection_R != Motor_Reverse)
    {
        printf("\ngo backward right");
        currentMotorDirection_R = Motor_Reverse;
        motor_reverse(Motor_FR, SPEED);
    }
    if(currentMotorDirection_L != Motor_Reverse)
    {
        printf("\ngo backward left");
        currentMotorDirection_L = Motor_Reverse;
        motor_reverse(Motor_FL, SPEED); 
    }
}

void stop()
{
    if(currentMotorDirection_R != Motor_Stopped)
    {
        printf("\nstop right");
        currentMotorDirection_R = Motor_Stopped;
        motor_stop(Motor_FR);
    }
    if(currentMotorDirection_L != Motor_Stopped)
    {
        printf("\nstop left");
        currentMotorDirection_L = Motor_Stopped;
        motor_stop(Motor_FL); 
    }
}