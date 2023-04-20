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

// const uint BLUE_LED_PIN = 15;
// const uint YELLOW_LED_PIN = 16;
// const uint GREEN_LED_PIN = 17;
const char WIFI_NETWORK_NAME[] = "PH1";
const char WIFI_PASSWORD[] = "12345678";

// monitor current state of motor so instructions are only sent for changes
volatile MotorDirection currentMotorDirection_L = Motor_Stopped;
volatile MotorDirection currentMotorDirection_R = Motor_Stopped;

const int SPEED = 40;

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
    
    // gpio_init(BLUE_LED_PIN);
    // gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    
    // gpio_init(YELLOW_LED_PIN);
    // gpio_set_dir(YELLOW_LED_PIN, GPIO_OUT);
    
    // gpio_init(GREEN_LED_PIN);
    // gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);

    dwm1001_init_communication();
    atmega_init_communication();

    motor_init_all();

    sleep_ms(2000);

    web_init(WIFI_NETWORK_NAME, WIFI_PASSWORD, "Arven", NULL, NULL, NULL);

    while (true) {
        if(scheduleId == -1)
        {
            web_request_check_schedule();
            // capture the schedule id once the web request has finished
            scheduleId = web_response_check_schedule();
        }
        else
        {
            if(numDoses <= 0) 
            {
                // capture the new dose amount once the web request has finished
                numDoses = web_response_retrieve_dose_stats();

                // calculate the new per-dose weight to be used when calculating the weight change
                if(numDoses > 0 && startingWeight > 0)
                {
                    Weight_DetermineDosage(startingWeight, numDoses);
                }
            }
        }

        // TODO: all of the code below should only run if schedule_id != -1
        // retrieve current frame 
        sensorValues = atmega_retrieve_sensor_values();

        if(!startingWeight && scheduleId != -1) 
        {
            startingWeight = Weight_CalculateMass(sensorValues.Weight);
            web_request_retrieve_dose_stats(scheduleId);
        }

        // only check the values below and act on them if SOMETHING has changed 
        // (also used as an indication of a proper frame received)
        if(sensorValues.Changes) 
        {        
            Weight_LoadState newLoadState = Weight_CheckForLoad(sensorValues.Weight); 

            if(newLoadState == Weight_LoadPresent && newLoadState != previousLoadState) 
            {
                Weight_Change change = Weight_CheckForChange(sensorValues.Weight);

                switch(change)
                {
                    case Weight_RefillChange:
                        
                        // determine the new starting weight
                        startingWeight = Weight_CalculateMass(sensorValues.Weight);
                        numDoses = 0;
                        web_request_retrieve_dose_stats(scheduleId);
                        break;
                    case Weight_LargeChange:
                        // TODO: log an event because this change was a possible concern
                    case Weight_SmallChange:
                        web_request_log_delivery(scheduleId);
                        break;
                    default:
                        break;
                }
            }
            // update the load state for later comparison (only if it's not an error)
            if(newLoadState != Weight_LoadError) previousLoadState = newLoadState;

            // check if there is an obstacle within 30cm
            bool obstacleLeft = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_L_Duration, 20);
            bool obstacleCentre = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_C_Duration, 20);
            bool obstacleRight = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_R_Duration, 20);

            // Check if the ground (50mm -- 5cm) is still there
            bool dropImminentLeft = IR_CheckForDrop(sensorValues.IR_L_Distance, 60); 
            bool dropImminentRight = IR_CheckForDrop(sensorValues.IR_R_Distance, 60);

            bool obstacleRear = sensorValues.Bump_L || sensorValues.Bump_R;
        
            //struct DWM1001_Position position = dwm1001_request_position();
            // TODO: retrieve the user location into somewhere in order to run comparisons against our position
            //web_request("/get_user_location");
            //pseudo navigation/object detection stuff

            //prioritize drop detection first
            if(!dropImminentLeft && !dropImminentRight){
                // printf("\nno drop");
                if(!obstacleCentre && !obstacleLeft && !obstacleRight && !obstacleRear) {
                    printf("\nno obstacle");
                    go_forward(); 
                } else {
                    //printf("\nobstacle");
                    //second priority is bump sensors, that way we know whether or not we can reverse
                    //reverse = okay
                    if(!obstacleRear){
                        //printf("\nno obstacle behind");
                        //third priority is whether or not something in front of arven is detected
                        //something in front = reverse left/right/straight, nothing in front = keep going straight
                        // and take turns depending on the UWB location
                        //
                        //no object behind, but there is an object in front
                        if(obstacleCentre){                         //depends on space between object and arven
                            if((!obstacleLeft && !obstacleRight) || (obstacleLeft && obstacleRight)){
                                printf("\nobstacle front");
                                //assuming Motor_FL = left, Motor_FR = right
                                //based on the xyz from the UWB module, we would set it to reverse left/right
                                //so we can go around the object, just setting it to reverse for now though
                                // motor_stop(Motor_FL);
                                // motor_stop(Motor_FR);

                                //wait x amount of seconds to make sure it stopped
                                go_backward();
                            }
                            //if an object is detected in front of arven and to the left, we would want it to reverse right
                            if(obstacleLeft && !obstacleRight){
                                printf("\nobstacle front and left");
                                // motor_stop(Motor_FL);
                                // motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                turn_right();
                            }
                            //if an object is detected in front of arven and to the right, we would want it to reverse left
                            if(!obstacleLeft && obstacleRight){
                                printf("\nobstacle front and right");
                                // motor_stop(Motor_FL);
                                // motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                turn_left();
                            }
                        //no object behind or in front
                        } else{
                            //if an object is detected left, and right, continue straight or reverse?
                            //probably depends on how much space is in between arven and the objects, etc
                            if(obstacleLeft && obstacleRight){
                                printf("\nobstacle left and right");
                                // motor_stop(Motor_FL);
                                // motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                go_forward();
                            }
                            //turn right if we see an object on the left
                            if(obstacleLeft && !obstacleRight){
                                printf("\nobstacle left");
                                // motor_stop(Motor_FL);
                                // motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                turn_right();                  
                            }
                            //turn left if there's only an object on the right
                            if(!obstacleLeft && obstacleRight){
                                printf("\nobstacle right");
                                // motor_stop(Motor_FL);
                                // motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                turn_left();   

                            }
                            //keep going forward if we detect no objects
                            if(!obstacleLeft && !obstacleRight){
                                printf("\nno obstacles");
                                go_forward();                      
                            }
                        }
                    }
                    //object detected behind
                    if(obstacleRear){
                        // printf("\nobstacle behind");
                        //not sure what to do if it's behind + in front? since you'd have to turn
                        //in such a way that there's a enough space in front/behind, which would 
                        //determine whether you reverse or go forward, etc
                        if(obstacleCentre){
                        // printf("\nobstacle centre");
                            //if it's surrounded, just stop? 
                            if(obstacleLeft && obstacleRight){
                                printf("\nobstacles everywhere");
                                stop();
                                //wait x amount of seconds to make sure it stopped
                            }
                            if(obstacleLeft && !obstacleRight){
                                printf("\nobstacle behind, front, and left");
                            }
                            if(!obstacleLeft && obstacleRight){
                                printf("\nobstacle behind, front, and right");
                            }                          
                        //something behind, nothing in front
                        } else{
                            // printf("\nno obstacle behind");
                            //nothing right/left
                            if(!obstacleLeft && !obstacleRight){
                                printf("\nobstacle behind");
                                go_forward();                             
                            }
                            if(obstacleLeft && !obstacleRight){
                                printf("\nobstacle left and behind");
                                // motor_stop(Motor_FL);
                                // motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                turn_right();                            
                            }
                            if(!obstacleLeft && obstacleRight){
                                printf("\nobstacle right and behind");
                                // motor_stop(Motor_FL);
                                // motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                turn_left();                            
                            }
                            //something behind, left, and right, go straight? depends on space
                            if(obstacleLeft && obstacleRight){
                                printf("\nobstacle left, right, behind");
                                // motor_stop(Motor_FL);
                                // motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                go_forward();                          
                            }
                        }
                    }
                }
            } else{
                printf("\ndrop");
                //just stop if we're about to fall
                stop();           
            }
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