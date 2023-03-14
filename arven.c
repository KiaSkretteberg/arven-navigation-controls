

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/pwm.h"
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
    // set motor pin to output (GP2, pin 4)
    gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);
    // Tell GPIO 3 , it is allocated to the PWM
    gpio_set_function(3, GPIO_FUNC_PWM);

    // Find out which PWM slice is connected to GPIO 3 (it's slice 0)
    uint slice_num = pwm_gpio_to_slice_num(0);

    // Set period of 4 cycles (0 to 3 inclusive)
    pwm_set_wrap(slice_num, 3);
    // Set channel A output high for one cycle before dropping
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 1);
    // Set the PWM running
    pwm_set_enabled(slice_num, true);
    //turn on the motor
        gpio_put(2, 1);
    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
}