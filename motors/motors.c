/*
 * motors.c
 *
 * Created: 2023-03-20
 * Author: Kia Skretteberg
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "motors.h"
#include "math.h"

/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

/// @brief Set the speed of the specified robot in cm/s (will calculate motor speed in rpms)
/// @param motor Which motor the speed is being set on
/// @param speed The intended speed of the robot in cm/s
int set_motor_speed(Motor motor, float speed);

/// @brief Set the direction of the specified motor to either forward or reverse
/// @param motor Which motor the direction is being set on
/// @param dir The direction the robot should move (forward or reverse)
void set_motor_direction(Motor motor, MotorDirection dir);

/// @brief Set the motor speed and direction all in one
/// @param motor Which motor the speed is being set on
/// @param speed The intended speed of the robot in cm/s
/// @param dir The direction the robot should move (forward or reverse)
void set_motor_dir_speed(Motor motor, float speed, MotorDirection dir);

/// @brief Get the specified pin type for the specified motor
/// @param motor Which motor the speed is being set on
/// @param pinType What type of pin should be retrieved (direction pin, or speed pin)
/// @return The pin value to be used in GPIO functions
int get_pin(Motor motor, MotorPinType pinType);

/// @brief allows setting a specific motor to specified frequency and duty.
/// Code adopted from https://www.i-programmer.info/programming/hardware/14849-the-pico-in-c-basic-pwm.html?start=2
// and mp_machine_pwm_freq_set from https://github.com/micropython/micropython/blob/master/ports/rp2/machine_pwm.c 
/// @param motor 
/// @param freq 
/// @param duty 
uint32_t set_pwm_frequency_duty(Motor motor, uint32_t freq, int duty);
 
/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

// The list of slice for the motors where the key is the motor number (enum), and the value is the slice number
volatile uint motor_slices[6];
volatile uint32_t period;

/************************************************************************/
/* Header Implementation                                                */
/************************************************************************/

void motor_init_all(void) 
{
    (void) motor_init(Motor_FL);
    (void) motor_init(Motor_FR);
}

uint motor_init(Motor motor) 
{
    uint motor_slice; // which pwm slice the motor pwm pin is located in
    int dirPin = get_pin(motor, Motor_PinType_Direction); //which pin to use for direction of motor (0/1) [F/R]
    int speedPin = get_pin(motor, Motor_PinType_Speed); // which pin to set for speed (enable) of motor (0-255)
    
    // set motor direction pin for the motor to Output
    gpio_init(dirPin);
    gpio_set_dir(dirPin, GPIO_OUT);
    // Set the Enable (speed) pin for the motor to PWM
    gpio_set_function(speedPin, GPIO_FUNC_PWM);
    // Find out which PWM slice motor is connected to, and save it for later reference
    motor_slices[motor] = pwm_gpio_to_slice_num(speedPin);
    // Set the frequency based on the MAX_RPM to ensure we can always go the speed we want to
    // ross suggested 10k as min, 100 is full duty, max speed
    set_pwm_frequency_duty(motor, 10000, 100); 
    return motor_slices[motor];
}

void motor_stop(Motor motor)
{
    pwm_set_enabled(motor_slices[motor], false);
}

void motor_forward(Motor motor, float speed)
{
    set_motor_dir_speed(motor, speed, Motor_Forward);
}

void motor_reverse(Motor motor, float speed)
{
    set_motor_dir_speed(motor, speed, Motor_Reverse);
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/

void set_motor_dir_speed(Motor motor, float speed, MotorDirection dir)
{
    set_motor_direction(motor, dir);
    set_motor_speed(motor, speed);
}

int set_motor_speed(Motor motor, float speed) 
{
    // a flag indicating if the speed exceeded the maximum value for RPM
    int maxed = 0;
    // Calculated RPM of the motor based on the desired speed (in cm, converted to m), pi, and wheel diameter
    int rpm = 60 * (speed / 100.0) / (M_PI * WHEEL_DIAMETER);
    // Desired duty to apply to motor based on the rpm and stored period
    int duty;

    // exceeded the max rpm, set to max
    if(rpm > MAX_RPM)
    {
        rpm = MAX_RPM;
        maxed = 1;
    }

    // Calculate duty as: (rpm/MAX_RPM) * period [but keep numerator large]
    duty = (rpm * period) / MAX_RPM;

    // TODO: Need a method to grab current encoder speed in order to maintain speed of motors after having set it?

    // Set the duty of the signal
    pwm_set_gpio_level(get_pin(motor, Motor_PinType_Speed), duty);
    // Set the PWM running (turn on the motor, at the set speed)
    pwm_set_enabled(motor_slices[motor], true);

    return maxed;
}

void set_motor_direction(Motor motor, MotorDirection dir)
{
    gpio_put(get_pin(motor, Motor_PinType_Direction), dir);
}

int get_pin(Motor motor, MotorPinType pinType)
{
    int dirPin; //which pin to use for direction of motor (0/1) [F/R]
    int speedPin; // which pin to set for speed (enable) of motor (0-255)

    switch(motor)
    {
        case Motor_FL:
            dirPin = M1;
            speedPin = EN1;
            break;
        case Motor_FR:
            dirPin = M2;
            speedPin = EN2;
            break;
    }

    switch(pinType)
    {
        case Motor_PinType_Direction:
            return dirPin;
        case Motor_PinType_Speed:
            return speedPin;
    }
}

/*
65535 is max value of divisor (65536 possible values)
16 bit
8 bit integer, 4 bit fractional (where are the other 4 bits?!?!?!)
255 is max for the 8 bit integer (256 possible values)
15 is the max for the 4 bit fraction (16 possible values)
*/
uint32_t set_pwm_frequency_duty(Motor motor, uint32_t freq, int duty)
{   
    // when divided by 16, gives 1, must be at least 1 (divide by 0 is bad!)
    const int min_divide = 16;
    // when divided by 16, gives 256, must be less than this because 255 15/16 is max
    const int max_divide = 4096;
    // clock speed of the pico, 125MHz
    uint32_t clock = 125000000;
    // pwm is 8.4bit -- meaning accuracy to 1/16th so we need to determine what to divide our clock by in order to get our frequency
    uint32_t clock_divide = clock / (freq * max_divide) + 
                            // always round up (add 0 or 1) to ensure we have a divider big enough for our desired frequency
                            (clock % (freq * max_divide) != 0);

    // if it's less than min, our frequency is too big (too fast), set to biggest
    if (clock_divide < min_divide)
        clock_divide = min_divide;
    // if it's bigger than max, frequency is too small (too slow), set to smallest
    else if(clock_divide >= max_divide)
        clock_divide = max_divide - 1;

    // calculate the period of the signal using the clock, frequency, and clock_divide
    // ???? What? 
    uint32_t period = clock * 16 / (clock_divide * freq) - 1;

    // Set the value to divide the clock by in order to achieve our frequency
    // the first chunk is a whole factor of 16 (must be at least 1)
    // the second chunk is the leftovers when divided by 16, 
    // e.g. 17 would produce 1, 1
    //      16 would produce 1, 0
    //      32 would produce 2, 0
    //      38 would produce 2, 6
    pwm_set_clkdiv_int_frac(motor_slices[motor], clock_divide/16,
                                        clock_divide & 0xF);
    // Set the period of the signal (duty must be < this value)
    pwm_set_wrap(motor_slices[motor], period);
    // Set the duty of the signal
    pwm_set_gpio_level(get_pin(motor, Motor_PinType_Speed), period * duty / 100);
    return period;
}