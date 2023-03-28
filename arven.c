#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/pwm.h"
#include "motors.h"
#include "dwm1001.h"
#include "atmega.h"
#include "weight.h"
#include "ultrasonic.h"
#include "ir.h"
//#include "pico/cyw43_arch.h"

const uint BLUE_LED_PIN = 15;
const uint YELLOW_LED_PIN = 16;
const uint GREEN_LED_PIN = 17;

int main() {
    struct AtmegaSensorValues sensorValues;

    stdio_init_all();
    // if (cyw43_arch_init()) {
    //     printf("Wi-Fi init failed");
    //     return -1;
    // }
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    
    gpio_init(YELLOW_LED_PIN);
    gpio_set_dir(YELLOW_LED_PIN, GPIO_OUT);
    
    gpio_init(GREEN_LED_PIN);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);

    dwm1001_init_communication();
    atmega_init_communication();

    motor_init_all();

    while (true) {
        // TODO: Remove. Just for proof of life purposes
        gpio_put(BLUE_LED_PIN, 1);
        sleep_ms(250);
        gpio_put(BLUE_LED_PIN, 0);
        sleep_ms(250);

        //retrieve current frame 
        sensorValues = atmega_retrieve_sensor_values();
        
        Weight_LoadState loadPresent = Weight_CheckForLoad(sensorValues.Weight); 

        // check if there is an obstacle within 30cm
        bool obstacleCentre = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_L_Duration, 30);
        bool obstacleLeft = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_C_Duration, 30);
        bool obstacleRight = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_R_Duration, 30);

        // Check if the ground (50mm -- 5cm) is still there
        bool dropImminentLeft = IR_CheckForDrop(sensorValues.IR_L_Distance, 50); 
        bool dropImminentRight = IR_CheckForDrop(sensorValues.IR_R_Distance, 50);

        bool obstacleRear = sensorValues.Bump_L || sensorValues.Bump_R;

        //dwm1001_request_position();

        //navigation testing stuff
        
        int turnSpeed = 30;
        int normalSpeed = 10;
        if(obstacleCentre){
            //if an object is detected ONLY in front of arven, we would want it to reverse only
            //if an object is detected in front and on the side, but not the back, we would want to reverse
            if((!obstacleLeft && !obstacleRight && !obstacleRear) || (obstacleLeft && obstacleRight && !obstacleRear)){
                //assuming Motor_FL = left, Motor_FR = right
                //based on the xyz from the UWB module, we would set it to reverse left/right
                //so we can go around the object, just setting it to reverse for now though
                motor_stop(Motor_FL);
                motor_stop(Motor_FR);
                //wait x amount of seconds to make sure it stopped
                motor_reverse(Motor_FL, turnSpeed); //not sure what speed to set it to?
                motor_reverse(Motor_FR, turnSpeed);
            }
            //if an object is detected in front of arven and to the left, we would want it to reverse right
            if(obstacleLeft && !obstacleRight && !obstacleRear){
                motor_stop(Motor_FL);
                motor_stop(Motor_FR);
                //wait x amount of seconds to make sure it stopped
                motor_reverse(Motor_FR, turnSpeed);
                motor_reverse(Motor_FL, normalSpeed); //stopping probably isn't what we want, but not sure how we would reverse in a
                                      //left/right direction?
            }
            //if an object is detected in front of arven and to the right, we would want it to reverse left
            if(!obstacleLeft && obstacleRight && !obstacleRear){
                motor_stop(Motor_FL);
                motor_stop(Motor_FR);
                //wait x amount of seconds to make sure it stopped
                motor_reverse(Motor_FR, normalSpeed);
                motor_reverse(Motor_FL, turnSpeed);
            }
            //if it's surrounded, just stop? 
            if(obstacleLeft && obstacleRight && obstacleRear){
                motor_stop(Motor_FL);
                motor_stop(Motor_FR);
            }
            //if an object is detected to in front and behind, not sure? 
            //would probably depend on the UWB stuff for whether we turn right/left
            //in terms of positioning
            if(!obstacleLeft && !obstacleRight && obstacleRear){
                //TODO
            }
            if(obstacleLeft && !obstacleRight && obstacleRear){
                motor_stop(Motor_FL);
                motor_stop(Motor_FR);
                //wait x amount of seconds to make sure it stopped
                motor_forward(Motor_FR, turnSpeed);
                motor_forward(Motor_FL, normalSpeed);                
            }
            if(!obstacleLeft && obstacleRight && obstacleRear){
                 motor_stop(Motor_FL);
                motor_stop(Motor_FR);
                //wait x amount of seconds to make sure it stopped
                motor_forward(Motor_FR, normalSpeed);
                motor_forward(Motor_FL, turnSpeed);                   
            }
        }
        else
        {
            motor_forward(Motor_FR, normalSpeed);
            motor_forward(Motor_FL, normalSpeed);
        }
       /*
        while(obstacleRight){
            obstacleLeft = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_L_Duration, normalSpeed);
            obstacleCentre = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_C_Duration, normalSpeed);
            obstacleRight = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_R_Duration, normalSpeed);
            //if an object is detected ONLY in front of arven, we would want it to turn left and forward
            if(!obstacleLeft && !obstacleCentre && !obstacleRear){
                //assuming Motor_FL = left, Motor_FR = right
                //based on the xyz from the UWB module, we would set it to reverse left/right
                //so we can go around the object, just setting it to reverse for now though
                motor_forward(Motor_FL, turnSpeed); //not sure what speed to set it to?
                motor_forward(Motor_FR, normalSpeed);
            }
            //if an object is detected to the right, and the left, we would want it to just continue straight without turning
            if(obstacleLeft && !obstacleCentre && !obstacleRear){
                motor_forward(Motor_FR, normalSpeed);
                motor_forward(Motor_FL, normalSpeed);
            }
            //if an object is detected to right, left and front, just reverse
            if(obstacleLeft && obstacleCentre && !obstacleRear){
                motor_reverse(Motor_FR, normalSpeed);
                motor_reverse(Motor_FL, normalSpeed);
            }
            //if it's surrounded, just stop? 
            if(obstacleLeft && obstacleCentre && obstacleRear){
                motor_stop(Motor_FL);
                motor_stop(Motor_FR);
            }
        } 
        */      
    }
}