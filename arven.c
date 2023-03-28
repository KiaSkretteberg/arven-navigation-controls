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
        /*
        //pseudo navigation/object detection stuff
        int turnSpeed = 15
        int normalSpeed = 10;
        //prioritize drop detection first
        if(!dropImminentLeft && ! dropImminentRight){
            //second priority is bump sensors, that way we know whether or not we can reverse
            if(!obstacleRear){
                if(obstacleCentre){
                    if((!obstacleLeft && !obstacleRight) || (obstacleLeft && obstacleRight)){
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
                        motor_stop(Motor_FL);
                        motor_stop(Motor_FR);
                        //wait x amount of seconds to make sure it stopped
                        motor_reverse(Motor_FR, turnSpeed);
                        motor_reverse(Motor_FL, normalSpeed); 
                    }
                    //if an object is detected in front of arven and to the right, we would want it to reverse left
                    if(!obstacleLeft && obstacleRight){
                        motor_stop(Motor_FL);
                        motor_stop(Motor_FR);
                        //wait x amount of seconds to make sure it stopped
                        motor_reverse(Motor_FR, normalSpeed);
                        motor_reverse(Motor_FL, turnSpeed);
                    }
                } else{
                    //if an object is detected on the left and right, continue straight?
                    //probably depends on how much space is in between arven and the objects
                    if(obstacleLeft && obstacleRight){
                        motor_stop(Motor_FL);
                        motor_stop(Motor_FR);
                        //wait x amount of seconds to make sure it stopped
                        motor_forward(Motor_FR, normalSpeed);
                        motor_forward(Motor_FL, normalSpeed);
                    }
                    //turn right if we see an object only on the left 
                    if(obstacleLeft && !obstacleRight){
                        motor_stop(Motor_FL);
                        motor_stop(Motor_FR);
                        //wait x amount of seconds to make sure it stopped
                        motor_forward(Motor_FR, turnSpeed);
                        motor_forward(Motor_FL, normalSpeed);                    
                    }
                    //turn left if there's only an object on the right
                    if(!obstacleLeft && obstacleRight){
                        motor_stop(Motor_FL);
                        motor_stop(Motor_FR);
                        //wait x amount of seconds to make sure it stopped
                        motor_forward(Motor_FR, normalSpeed);
                        motor_forward(Motor_FL, turnSpeed);    

                    }
                    //keep going forward if we detect no objects
                    if(!obstacleLeft && !obstacleRight){
                        motor_stop(Motor_FL);
                        motor_stop(Motor_FR);
                        //wait x amount of seconds to make sure it stopped
                        motor_forward(Motor_FR, normalSpeed);
                        motor_forward(Motor_FL, normalSpeed);                        
                    }
                }
 
   
            }
            if(obstacleRear){
                //if it's surrounded, just stop? 
                if(obstacleLeft && obstacleRight && obstacleCentre){
                    motor_stop(Motor_FL);
                    motor_stop(Motor_FR);
                }
                //if an object is detected to in front and behind, not sure? 
                //would probably depend on the UWB stuff for whether we turn right/left
                //in terms of positioning
                if(!obstacleLeft && !obstacleRight && obstacleCentre){
                    //TODO
                }
                //
                if(obstacleLeft && !obstacleRight && obstacleCentre){
                    motor_stop(Motor_FL);
                    motor_stop(Motor_FR);
                    //wait x amount of seconds to make sure it stopped
                    motor_forward(Motor_FR, turnSpeed);
                    motor_forward(Motor_FL, normalSpeed);                
                }
                if(!obstacleLeft && obstacleRight && obstacleCentre){
                    motor_stop(Motor_FL);
                    motor_stop(Motor_FR);
                    //wait x amount of seconds to make sure it stopped
                    motor_forward(Motor_FR, normalSpeed);
                    motor_forward(Motor_FL, turnSpeed);                   
                }

            }
        } else{
            //just stop if we're about to fall
            motor_stop(Motor_FL);
            motor_stop(Motor_FR);            
        }
        */
    }
}