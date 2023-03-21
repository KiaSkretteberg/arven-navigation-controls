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

        // // checek if there is an obstacle within 10cm
        // bool obstacleLeft = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_L_Duration, 10);
        // bool obstacleCentre = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_C_Duration, 10);
        // bool obstacleRight = Ultrasonic_CheckForObstacle(sensorValues.Ultrasonic_R_Duration, 10);

        // // Check if the ground (50mm -- 5cm) is still there
        // bool dropImminentLeft = IR_CheckForDrop(sensorValues.IR_L_Distance, 50); 
        // bool dropImminentRight = IR_CheckForDrop(sensorValues.IR_R_Distance, 50);

        // bool obstacleRear = sensorValues.Bump_L || sensorValues.Bump_R;

        // dwm1001_request_position();
    }
}