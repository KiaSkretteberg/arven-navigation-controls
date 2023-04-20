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

typedef enum
{
    RobotState_Idle,
    RobotState_NavigatingToUser,
    RobotState_Stuck,
    RobotState_DeliveringPayload,
    RobotState_NavigatingHome
} RobotState;

typedef enum
{
    MotionState_Forward,
    MotionState_Reverse,
    MotionState_Stop,
    MotionState_TurnRight,
    MotionState_TurnLeft,
    MotionState_ToBeDetermined
} MotionState;

typedef enum
{
    NavigationResult_Success,
    NavigationResult_Failure,
    NavigationResult_Stuck
} NavigationResult;


/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

const char WIFI_NETWORK_NAME[] = "PH1";
const char WIFI_PASSWORD[] = "12345678";

// monitor current state of motor so instructions are only sent for changes
volatile MotorDirection currentMotorDirection_L = Motor_Stopped;
volatile MotorDirection currentMotorDirection_R = Motor_Stopped;

const int SPEED = 40;

// user's current position, checked every 500ms
volatile struct DWM1001_Position userPosition;

/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

bool navigating_to_user(struct AtmegaSensorValues sensorValues);
void navigating_home(struct AtmegaSensorValues sensorValues);
void delivering_payload(struct AtmegaSensorValues sensorValues);
MotionState interpret_sensors(struct AtmegaSensorValues sensorValues);
void turn_right();
void turn_left();
void go_forward();
void go_backward();
void stop();

int main() {
    RobotState robotState = RobotState_Idle;
    int scheduleId = -1; //may be populated during operation if a schedule is operating

    // Navigating state variables
    struct AtmegaSensorValues sensorValues;

    // Delivery payload state variables
    //TODO:
    
    stdio_init_all();

    dwm1001_init_communication();
    atmega_init_communication();
    motor_init_all();

    // wait 2seconds to allow debugging connection
    sleep_ms(2000);

    web_init(WIFI_NETWORK_NAME, WIFI_PASSWORD, "Arven", NULL, NULL, NULL);

    while (true) 
    {
        switch(robotState)
        {
            case RobotState_Idle:
                web_request_check_schedule();
                // capture the schedule id once the web request has finished
                scheduleId = web_response_check_schedule();
                if(scheduleId != -1)
                    robotState = RobotState_NavigatingToUser;
                break;
            case RobotState_NavigatingToUser:
                // get the frame info from the atmega
                // TODO: We should toggle control of the atmega code detecting the sensors based on if we want data
                sensorValues = atmega_retrieve_sensor_values();

                bool done = navigating_to_user(sensorValues);
                // if(done)
                //     robotState = DeliveringPayload;
                break;
            case RobotState_DeliveringPayload:
                // get the frame info from the atmega
                // TODO: We should toggle control of the atmega code detecting the sensors based on if we want data
                sensorValues = atmega_retrieve_sensor_values();

                delivering_payload(sensorValues);
                break;
            case RobotState_NavigatingHome:
                // get the frame info from the atmega
                // TODO: We should toggle control of the atmega code detecting the sensors based on if we want data
                sensorValues = atmega_retrieve_sensor_values();

                navigating_home(sensorValues);
                break;
        }
    }
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/

bool navigating_to_user(struct AtmegaSensorValues sensorValues)
{
    bool done = false;
    MotionState state = interpret_sensors(sensorValues);
    if(state == MotionState_Stop)
    {
        stop();
    }
    else
    {
        // struct DWM1001_Position position = dwm1001_request_position();
        // //TODO: check user's position periodically
        // if(!userPosition.set) {
        //     web_request_get_user_location();
        //     userPosition = web_response_get_user_location();
        // }

        // printf("\npositionSet: %i", position.set);
        // printf("\nuserPositionSet: %i", userPosition.set);

        //  if (position.set && userPosition.set && 
        //     position.x != userPosition.x && 
        //     position.y != userPosition.y &&
        //     position.z != userPosition.z)
        // {
            switch(state)
            {
                case MotionState_Forward:
                    go_forward();
                    break;
                case MotionState_Reverse:
                    go_backward();
                    break;
                case MotionState_TurnRight:
                    turn_right();
                    break;
                case MotionState_TurnLeft:
                    turn_left();
                    break;
                case MotionState_ToBeDetermined:
                    //TODO: Decide what this should do
                    turn_right();
                    break;
            }
        // }
        // else
        // {
        //     done = true;
        //     stop();
        // }
    }   
    return done;
}

void navigating_home(struct AtmegaSensorValues sensorValues)
{
    MotionState state = interpret_sensors(sensorValues);
    if(state == MotionState_Stop)
    {
        stop();
    }
    else
    {
        struct DWM1001_Position position = dwm1001_request_position();

         if (position.set && 
            position.x != 0 && 
            position.y != 0 &&
            position.z != 0)
        {
            switch(state)
            {
                case MotionState_Forward:
                    go_forward();
                    break;
                case MotionState_Reverse:
                    go_backward();
                    break;
                case MotionState_TurnRight:
                    turn_right();
                    break;
                case MotionState_TurnLeft:
                    turn_left();
                    break;
                case MotionState_ToBeDetermined:
                    //TODO: Decide what this should do
                    turn_right();
                    break;
            }
        }
    }   
}

void delivering_payload(struct AtmegaSensorValues sensorValues)
{
    // keep track of previous state for the weight sensor so we can detect when it changes
    Weight_LoadState previousLoadState = Weight_LoadUninitialized;
    // track the starting weight and number of doses available for use in calculating per-dose weight
    float startingWeight = -1;
    // int numDoses = 0;
    // if(numDoses <= 0) 
    // {
    //     // capture the new dose amount once the web request has finished
    //     numDoses = web_response_retrieve_dose_stats();

    //     // calculate the new per-dose weight to be used when calculating the weight change
    //     if(numDoses > 0 && startingWeight > 0)
    //     {
    //         float doseWeight = Weight_DetermineDosage(startingWeight, numDoses);
    //         printf("\ndoseWeight: %f", doseWeight);
    //     }
    // }

    // if we don't have a starting weight (and we're not unitialized), log the current weight as the starting weight
    // IGNORE: then determine how much a single dose should weigh based on amount of doses remaining
    /*if((previousLoadState != Weight_LoadUninitialized && startingWeight == -1) || startingWeight == 0) 
    {
        startingWeight = Weight_CalculateMass(sensorValues.Weight);
        printf("\nstartingWeight: %f", startingWeight);
        //web_request_retrieve_dose_stats(scheduleId);
    }

    // Check if we currently have something on the weight sensor
    Weight_LoadState newLoadState = Weight_CheckForLoad(sensorValues.Weight); 

    // if the state changed from something removed, to something added, 
    // IGNORE: check to see how much the weight changed from the last time there was weight
    if (previousLoadState != Weight_LoadUninitialized && startingWeight > 0 && 
        newLoadState == Weight_LoadPresent && newLoadState != previousLoadState) 
    {
        printf("\ndose taken");
        web_request_log_delivery(scheduleId);
        // check how big the change was
        // Weight_Change change = Weight_CheckForChange(sensorValues.Weight);

        // switch(change)
        // {
        //     // the change increased the weight so the medication was refilled
        //     case Weight_RefillChange:
        //         printf("\nRefill triggered");
        //         // determine the new starting weight
        //         startingWeight = Weight_CalculateMass(sensorValues.Weight);
        //         numDoses = 0;
        //         //TODO: The api requests needs to log a refill somehow
        //         web_request_retrieve_dose_stats(scheduleId);
        //         break;
        //     // the change was too big and may indicate an overdose, but we'll still log a delivery
        //     case Weight_LargeChange:
        //         printf("\nlarge change detected");
        //         // TODO: log an event because this change was a possible concern
        //     // the change was normal, just log a delivery
        //     case Weight_SmallChange:
        //         printf("\nnormal dose detected");
        //         web_request_log_delivery(scheduleId);
        //         break;
        //     // any other change doesn't matter and may have been a hiccup
        //     default:
        //         break;
        // }
    }
    // update the load state for later comparison (only if it's not an error)
    if(newLoadState != Weight_LoadError) previousLoadState = newLoadState;
*/
}

MotionState interpret_sensors(struct AtmegaSensorValues sensorValues)
{
    MotionState action = MotionState_ToBeDetermined;

    // check if there is an obstacle within 30cm
    bool obstacleLeft = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_L_Duration, 30);
    bool obstacleCentre = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_C_Duration, 30);
    bool obstacleRight = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_R_Duration, 30);

    // Check if the ground (60mm -- 6cm) is still there
    bool dropImminentLeft = IR_CheckForDrop(sensorValues.IR_L_Distance, 60); 
    bool dropImminentRight = IR_CheckForDrop(sensorValues.IR_R_Distance, 60);

    // Check if any of the backup sensors are being pressed
    bool obstacleRear = sensorValues.Bump_L || sensorValues.Bump_R;    

    if(dropImminentLeft || dropImminentRight)
    {
        action = MotionState_Stop;
        //TODO: We may want to backup/turn if only one has a drop pending  
        //TODO: If we sit here too long, report a failed delivery         
    }
    // no drop, proceed to check other sensors
    else
    {
        // no obstacles in front, so go forward
        if(!obstacleCentre && !obstacleLeft && !obstacleRight) 
        {
            printf("\nno obstacles");
            action = MotionState_Forward; 
        } 
        // obstacle directly in front, and to the left, so turn right
        else if(obstacleCentre && obstacleLeft && !obstacleRight)
        {
             printf("\nfront and left obstacles");
            action = MotionState_TurnRight;
        }
        // obstacle directly in front, and to the right, so turn left
        else if (obstacleCentre && obstacleRight && !obstacleLeft)
        {
             printf("\nfront and right obstacles");
            action = MotionState_TurnLeft;
        }
        // just an obstacle in front, so turn towards the destination
        else if (obstacleCentre && !obstacleRight && !obstacleLeft)
        {
            printf("\nfront obstacle");
            action = MotionState_ToBeDetermined;
        }
        // obstacles all in front, but no rear, so backup
        else if (obstacleCentre && obstacleRight && obstacleLeft && !obstacleRear)
        {
            printf("\nfront/left/right obstacles only");
            action = MotionState_Reverse;
        }
        // obstacles everywhere, just stop
        else
        {
            printf("\nall the obstacles");
            action = MotionState_Stop;
            //TODO: If we sit here too long, report a failed delivery
        }
    }    
    return action;
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