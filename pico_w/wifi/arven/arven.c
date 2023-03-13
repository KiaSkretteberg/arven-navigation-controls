

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"

#define BLUE_LED_PIN 20

int main() {
    stdio_init_all();
    // if (cyw43_arch_init()) {
    //     printf("Wi-Fi init failed");
    //     return -1;
    // }
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    while (true) {
        gpio_put(BLUE_LED_PIN, 1);
        sleep_ms(250);
        gpio_put(BLUE_LED_PIN, 0);
        sleep_ms(250);
    }
}