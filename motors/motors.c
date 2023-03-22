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
 
/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

// The list of slice for the motors where the key is the motor number (enum), and the value is the slice number
volatile uint motor_slices[6];

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
    int dirPin = get_pin(motor, Motor_PinType_Direction); //which pin to use for direction of motor (0/1) [F/R]
    int speedPin = get_pin(motor, Motor_PinType_Speed); // which pin to set for speed (enable) of motor (0-255)
    
    // set motor direction pin for the motor to Output
    gpio_init(dirPin);
    gpio_set_dir(dirPin, GPIO_OUT);
    // Set the Enable (speed) pin for the motor to PWM
    gpio_set_function(speedPin, GPIO_FUNC_PWM);
    // Find out which PWM slice motor is connected to, and save it for later reference
    motor_slices[motor] = pwm_gpio_to_slice_num(speedPin);
    // Set the warp for the signal (effectively the period) -- this will be 1 less than max duty
    pwm_set_wrap(motor_slices[motor], MOTOR_PERIOD - 1);
    // set divisor to largest possible (255) in order to get slowest possible frequency (~1.9kHz)
    pwm_set_clkdiv_int_frac(motor_slices[motor], 27, 0); // calculated as 125k / freq / 16 / 16
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
    duty = (rpm * MOTOR_PERIOD) / MAX_RPM;

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
    return -1;
}