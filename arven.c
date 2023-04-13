#include <stdio.h>
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
const char DEVICE_SERIAL[] = "RX-AR2023-0001";

int main() {
    struct AtmegaSensorValues sensorValues;
    Weight_LoadState previousLoadState = Weight_LoadNotPresent;
    int schedule_id = 0; //may be populated during operation if a schedule is operating

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

    //web_init(WIFI_NETWORK_NAME, WIFI_PASSWORD, "Arven", NULL, NULL, NULL);

    // TODO: Only run the navigation code if we have a schedule returned (schedule_id != 0)
    //schedule_id = web_request("/check_schedule");

    while (true) {
        char buff[20];
        //retrieve current frame 
        sensorValues = atmega_retrieve_sensor_values();

        // only check the values below and act on them if SOMETHING has changed 
        // (also used as an indication of a proper frame received)
        if(sensorValues.Changes) 
        {        
            // Weight_LoadState newLoadState = Weight_CheckForLoad(sensorValues.Weight); 
            // if(newLoadState == Weight_LoadPresent && newLoadState != previousLoadState) {
            //     Weight_CheckForChange(sensorValues.Weight, 10);
            //     // update the load state for later comparison
            //     previousLoadState = newLoadState;
            //      //TODO: log the delivery if the change indicates it was taken
            //      //web_request("log_delivery/device/"+DEVICE_SERIAL+"/schedule_id/"+schedule_id);
            // }

            // check if there is an obstacle within 30cm
            bool obstacleLeft = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_L_Duration, 30);
            bool obstacleCentre = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_C_Duration, 30);
            bool obstacleRight = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_R_Duration, 30);

            // Check if the ground (50mm -- 5cm) is still there
            bool dropImminentLeft = IR_CheckForDrop(sensorValues.IR_L_Distance, 50); 
            bool dropImminentRight = IR_CheckForDrop(sensorValues.IR_R_Distance, 50);

            bool obstacleRear = sensorValues.Bump_L || sensorValues.Bump_R;
        
            struct DWM1001_Position position = dwm1001_request_position();
            // TODO: retrieve the user location into somewhere in order to run comparisons against our position
            //web_request("/get_user_location");
            //pseudo navigation/object detection stuff
            int turnSpeed = 25;
            int normalSpeed = 20;  

            //prioritize drop detection first
            if(!dropImminentLeft && !dropImminentRight){
                // printf("\nno drop");
                if(!obstacleCentre && !obstacleLeft && !obstacleRight && !obstacleRear) {
                    printf("\nno obstacle");
                    motor_forward(Motor_FR, normalSpeed);
                    motor_forward(Motor_FL, normalSpeed); 
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
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_reverse(Motor_FL, turnSpeed); 
                                motor_reverse(Motor_FR, turnSpeed);
                            }
                            //if an object is detected in front of arven and to the left, we would want it to reverse right
                            if(obstacleLeft && !obstacleRight){
                                printf("\nobstacle front and left");
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_reverse(Motor_FR, turnSpeed);
                                motor_reverse(Motor_FL, normalSpeed); 
                            }
                            //if an object is detected in front of arven and to the right, we would want it to reverse left
                            if(!obstacleLeft && obstacleRight){
                                printf("\nobstacle front and right");
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_reverse(Motor_FR, normalSpeed);
                                motor_reverse(Motor_FL, turnSpeed);
                            }
                        //no object behind or in front
                        } else{
                            //if an object is detected left, and right, continue straight or reverse?
                            //probably depends on how much space is in between arven and the objects, etc
                            if(obstacleLeft && obstacleRight){
                                printf("\nobstacle left and right");
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_forward(Motor_FR, normalSpeed);
                                motor_forward(Motor_FL, normalSpeed);
                            }
                            //turn right if we see an object on the left
                            if(obstacleLeft && !obstacleRight){
                                printf("\nobstacle left");
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_forward(Motor_FR, turnSpeed);
                                motor_forward(Motor_FL, normalSpeed);                    
                            }
                            //turn left if there's only an object on the right
                            if(!obstacleLeft && obstacleRight){
                                printf("\nobstacle right");
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_forward(Motor_FR, normalSpeed);
                                motor_forward(Motor_FL, turnSpeed);    

                            }
                            //keep going forward if we detect no objects
                            if(!obstacleLeft && !obstacleRight){
                                printf("\nno obstacles");
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_forward(Motor_FR, normalSpeed);
                                motor_forward(Motor_FL, normalSpeed);                        
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
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
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
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_forward(Motor_FR, normalSpeed);
                                motor_forward(Motor_FL, normalSpeed);                              
                            }
                            if(obstacleLeft && !obstacleRight){
                                printf("\nobstacle left and behind");
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_forward(Motor_FR, turnSpeed);
                                motor_forward(Motor_FL, normalSpeed);                            
                            }
                            if(!obstacleLeft && obstacleRight){
                                printf("\nobstacle right and behind");
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_forward(Motor_FL, turnSpeed);
                                motor_forward(Motor_FR, normalSpeed);                            
                            }
                            //something behind, left, and right, go straight? depends on space
                            if(obstacleLeft && obstacleRight){
                                printf("\nobstacle left, right, behind");
                                motor_stop(Motor_FL);
                                motor_stop(Motor_FR);
                                //wait x amount of seconds to make sure it stopped
                                motor_forward(Motor_FR, normalSpeed);
                                motor_forward(Motor_FL, normalSpeed);                            
                            }
                        }
                    }
                }
            } else{
                printf("\ndrop");
                //just stop if we're about to fall
                motor_stop(Motor_FL);
                motor_stop(Motor_FR);            
            }
        }
    }
}