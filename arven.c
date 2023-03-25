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

const uint LED_PIN = 15;
int main() {
    struct AtmegaSensorValues sensorValues;

    stdio_init_all();
    // if (cyw43_arch_init()) {
    //     printf("Wi-Fi init failed");
    //     return -1;
    // }
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    dwm1001_init_communication();
    atmega_init_communication();

    motor_init_all();

    
    motor_forward(Motor_FR, 20);

    while (true) {
        // TODO: Remove. Just for proof of life purposes
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);

        // //retrieve current frame 
        // sensorValues = atmega_retrieve_sensor_values();
        
        // Weight_LoadState loadPresent = Weight_CheckForLoad(sensorValues.Weight); 

        // // check if there is an obstacle within 10cm
        // bool obstacleLeft = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_L_Duration, 10);
        // bool obstacleCentre = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_C_Duration, 10);
        // bool obstacleRight = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_R_Duration, 10);

        // // Check if the ground (50mm -- 5cm) is still there
        // bool dropImminentLeft = IR_CheckForDrop(sensorValues.IR_L_Distance, 50); 
        // bool dropImminentRight = IR_CheckForDrop(sensorValues.IR_R_Distance, 50);

        // bool obstacleRear = sensorValues.Bump_L || sensorValues.Bump_R;

        //dwm1001_request_position();

        //navigation testing stuff
        
        // while(obstacleCentre){
        //     obstacleLeft = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_L_Duration, 10);
        //     obstacleCentre = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_C_Duration, 10);
        //     obstacleRight = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_R_Duration, 10);
        //     //if an object is detected ONLY in front of arven, we would want it to reverse only
        //     //if an object is detected in front and on the side, but not the back, we would want to reverse
        //     if((!obstacleLeft && !obstacleRight && !bumpSensors) || (obstacleLeft && obstacleRight && !bumpSensors)){
        //         //assuming M1 = left, M2 = right
        //         //based on the xyz from the UWB module, we would set it to reverse left/right
        //         //so we can go around the object, just setting it to reverse for now though
        //         motor_reverse(M1, 30); //not sure what speed to set it to?
        //         motor_reverse(M2, 30);
        //     }
        //     //if an object is detected in front of arven and to the left, we would want it to reverse right
        //     if(obstacleLeft && !obstacleRight && !bumpSensors){
        //         motor_reverse(M2, 30);
        //         motor_reverse(M1, 10); //stopping probably isn't what we want, but not sure how we would reverse in a
        //                               //left/right direction?
        //     }
        //     //if an object is detected in front of arven and to the right, we would want it to reverse left
        //     if(!obstacleLeft && obstacleRight && !bumpSensors){
        //         motor_reverse(M2, 10);
        //         motor_reverse(M1, 30);
        //     }
        //     //if it's surrounded, just stop? 
        //     if(obstacleLeft && obstacleRight && bumpSensors){
        //         motor_stop(M1);
        //         motor_stop(M2);
        //     }
        // }
    }
}