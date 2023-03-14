#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/pwm.h"
#include "motors.h"
//#include "pico/cyw43_arch.h"

const uint LED_PIN = 15;

int main() {
    stdio_init_all();
    // if (cyw43_arch_init()) {
    //     printf("Wi-Fi init failed");
    //     return -1;
    // }
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    motor_init_all();


    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
}