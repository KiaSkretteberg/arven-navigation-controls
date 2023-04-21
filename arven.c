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
    NavigationResult_Complete,
    NavigationResult_Incomplete,
    NavigationResult_Stuck
} NavigationResult;

typedef enum
{
    DeliveryState_WaitingRemoval,
    DeliveryState_Removed,
    DeliveryState_Complete
} DeliveryState;


/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

// Wifi details
const char WIFI_NETWORK_NAME[] = "PH1";
const char WIFI_PASSWORD[] = "12345678";

// Motor speed, in cm/s
const int SPEED = 40;

// How long the robot can be "stopped" before it's considered stuck
const int STUCK_DURATION = 60000; //1 minute (60s ==> 60,000ms) TODO: This isn't actually true

// How long the weight sensor must be in the same state before it will transition between states
const int WEIGHT_DURATION = 500;

// monitor current state of motor so instructions are only sent for changes
volatile MotionState currentRobotMotionState = MotionState_ToBeDetermined;

/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

NavigationResult navigating_to_user(struct AtmegaSensorValues sensorValues);
NavigationResult navigating_home(struct AtmegaSensorValues sensorValues);
bool delivering_payload(struct AtmegaSensorValues sensorValues, int scheduleId);
MotionState interpret_sensors(struct AtmegaSensorValues sensorValues);
NavigationResult navigate(struct AtmegaSensorValues sensorValues, struct DWM1001_Position destination);
void act_on_motion_state(MotionState action);
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
    
    stdio_init_all();

    dwm1001_init_communication();
    atmega_init_communication();
    motor_init_all();

    // wait 2seconds to allow debugging connection
    sleep_ms(2000);

    web_init(WIFI_NETWORK_NAME, WIFI_PASSWORD, "Arven", NULL, NULL, NULL);

    while (true) 
    {
        if (robotState == RobotState_NavigatingHome ||
            robotState == RobotState_DeliveringPayload ||
            robotState == RobotState_NavigatingToUser)
        {
            // get the frame info from the atmega
            // TODO: We should toggle control of the atmega code detecting the sensors based on if we want data
            sensorValues = atmega_retrieve_sensor_values();
        }

        switch(robotState)
        {
            case RobotState_Idle:
                scheduleId = idle();
                if(scheduleId != -1)
                    robotState = RobotState_NavigatingToUser;
                break;
            case RobotState_NavigatingToUser:
                NavigationResult result = navigating_to_user(sensorValues);
                if(result == NavigationResult_Complete)
                    robotState = RobotState_DeliveringPayload;
                else if (result == NavigationResult_Stuck)
                    robotState = RobotState_Stuck;
                break;
            case RobotState_DeliveringPayload:
                if(delivering_payload(sensorValues, scheduleId))
                    robotState = RobotState_NavigatingHome;
                break;
            case RobotState_NavigatingHome:
                NavigationResult result = navigating_home(sensorValues);
                if(result == NavigationResult_Complete)
                    robotState = RobotState_Idle;
                else if (result == NavigationResult_Stuck)
                    robotState = RobotState_Stuck;
                break;
        }
    }
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/

int idle()
{
    web_request_check_schedule();
    // capture the schedule id once the web request has finished
    return web_response_check_schedule();
}

NavigationResult navigating_to_user(struct AtmegaSensorValues sensorValues)
{
    static struct DWM1001_Position userPosition;
    //TODO: How to set "set" only the first time?
    userPosition.set = 0;
    // //TODO: check user's position periodically (every 500ms)
    // if(!userPosition.set) {
    //     web_request_get_user_location();
    //     userPosition = web_response_get_user_location();
    // }

    // printf("\npositionSet: %i", position.set);
    // printf("\nuserPositionSet: %i", userPosition.set);

    return navigate(sensorValues, userPosition);
}

NavigationResult navigating_home(struct AtmegaSensorValues sensorValues)
{
    struct DWM1001_Position homePosition;
    homePosition.x = 0;
    homePosition.y = 0;
    homePosition.z = 0;
    homePosition.set = 1;

    return navigate(sensorValues, homePosition);
}

bool delivering_payload(struct AtmegaSensorValues sensorValues, int scheduleId)
{
    static DeliveryState state = DeliveryState_WaitingRemoval;
    static int loadStateCount = 0;
    bool complete = 0;
    // Check if we currently have something on the weight sensor
    Weight_LoadState loadState = Weight_CheckForLoad(sensorValues.Weight); 

    switch(state)
    {
        case DeliveryState_WaitingRemoval:
            if(loadState == Weight_LoadNotPresent)
            {
                ++loadStateCount;
                if(loadStateCount >= WEIGHT_DURATION)
                {
                    state = DeliveryState_Removed;
                }
            }
            else
            {
                loadStateCount = 0;
            }
            break;
        case DeliveryState_Removed:
            if(loadState == Weight_LoadPresent)
            {
                ++loadStateCount;
                if(loadStateCount >= WEIGHT_DURATION)
                {
                    state = DeliveryState_Complete;
                }
            }
            else
            {
                loadStateCount = 0;
            }
            break;
        case DeliveryState_Complete:
            if(loadState == Weight_LoadPresent)
            {
                ++loadStateCount;
                if(loadStateCount >= WEIGHT_DURATION)
                {
                    printf("\ndose taken");
                    state = DeliveryState_WaitingRemoval;
                    web_request_log_delivery(scheduleId);
                    complete = 1;
                }
            }
            else
            {
                loadStateCount = 0;
            }
            break;
    }

    return complete;
}

NavigationResult navigate(struct AtmegaSensorValues sensorValues, struct DWM1001_Position destinationPosition)
{
    NavigationResult result = NavigationResult_Incomplete;
    static stoppedCount = 0;

    MotionState state = interpret_sensors(sensorValues);
    if(state == MotionState_Stop)
    {
        currentRobotMotionState = MotionState_Stop;
        stop();
        ++stoppedCount;
        if(stoppedCount == STUCK_DURATION)
            result = NavigationResult_Stuck;
    }
    else
    {
        //reset the stop count
        stoppedCount = 0;
        
        // struct DWM1001_Position robotPosition = dwm1001_request_position();
        long xDiff = robotPosition.x - destinationPosition.x;
        long yDiff = robotPosition.y - destinationPosition.y;
        long zDiff = robotPosition.z - destinationPosition.z;
        //basically we would want to find the difference between the two points, so
        //we would take the absolute value of the difference between the two since we
        //don't care about magnitude when it comes to how close they are to each other
        //if (robotPosition.set && destinationPosition.set && 
        //     abs(xDiff) >= 20 && abs(yDiff) >= 20 && abs(zDiff) >= 20)
        // {
            switch(state)
            {
                case MotionState_Forward:
                case MotionState_Reverse:
                case MotionState_TurnRight:
                case MotionState_TurnLeft:
                    act_on_motion_state(state);
                    break;
                case MotionState_ToBeDetermined:
                    currentRobotMotionState = MotionState_TurnRight;
                    //TODO: Decide what this should do
                    if(xDiff < 0){
                        turn_right();
                    } else{
                        turn_left();
                    }                   
                    break;
            }
        // }
        // else
        // {
        //     result = NavigationResult_Complete;
        //     stop();
        // }
    }   
    return result;
}

void act_on_motion_state(MotionState action)
{
    currentRobotMotionState = action;

    switch(action)
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
    }
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
    if(currentRobotMotionState != MotionState_TurnRight)
    {
        printf("\ngo backward right");
        motor_reverse(Motor_FR, SPEED);
        printf("\ngo forward left");
        motor_forward(Motor_FL, SPEED); 
    }
}

void turn_left()
{
    if(currentRobotMotionState != MotionState_TurnLeft)
    {
        printf("\ngo forward right");
        motor_forward(Motor_FR, SPEED);
        printf("\ngo backward left");
        motor_reverse(Motor_FL, SPEED); 
    }
}

void go_forward()
{
    if(currentRobotMotionState != MotionState_Forward)
    {
        printf("\ngo forward right");
        motor_forward(Motor_FR, SPEED);
        printf("\ngo forward left");
        motor_forward(Motor_FL, SPEED); 
    }
}

void go_backward()
{
    if(currentRobotMotionState != MotionState_Reverse)
    {
        printf("\ngo backward right");
        motor_reverse(Motor_FR, SPEED);
        printf("\ngo backward left");
        motor_reverse(Motor_FL, SPEED); 
    }
}

void stop()
{
    if(currentRobotMotionState != MotionState_Stop)
    {
        printf("\nstop right");
        motor_stop(Motor_FR);
        printf("\nstop left");
        motor_stop(Motor_FL); 
    }
}