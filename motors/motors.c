#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "motors.h"

// The list of slice for the motors where the key is the motor number (enum), and the value is the slice number
volatile uint motor_slices[6];

/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

// Set the speed of the specified motor by setting the duty as a max of MOTOR_PERIOD
void set_motor_speed(Motor motor, char speed);
// Set the direction of the specified motor to either forward or reverse
void set_motor_direction(Motor motor, MotorDirection dir);
// Set the motor speed and direction all in one
void set_motor_dir_speed(Motor motor, char speed, MotorDirection dir);
// Get the specified pin type for the specified motor
int get_pin(Motor motor, MotorPinType pinType);


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
    // Set period: 4 cycles (0 to 3 inclusive)
    pwm_set_wrap(motor_slices[motor], MOTOR_PERIOD);
    return motor_slices[motor];
}

void motor_stop(Motor motor)
{
    pwm_set_enabled(motor_slices[motor], false);
}

void motor_forward(Motor motor, char speed)
{
    set_motor_dir_speed(motor, speed, Motor_Forward);
}

void motor_reverse(Motor motor, char speed)
{
    set_motor_dir_speed(motor, speed, Motor_Reverse);
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/

void set_motor_dir_speed(Motor motor, char speed, MotorDirection dir)
{
    set_motor_direction(motor, dir);
    set_motor_speed(motor, speed);
}

void set_motor_speed(Motor motor, char speed) 
{
    if (speed > MOTOR_PERIOD)
        speed = MOTOR_PERIOD;

    // Set period
    //TODO: Do something with speed to do with period and duty
    pwm_set_wrap(motor_slices[motor], MOTOR_PERIOD);
    // set duty
    pwm_set_gpio_level(get_pin(motor, Motor_PinType_Speed), speed);
    // Set the PWM running (turn on the motor, at the set speed)
    pwm_set_enabled(motor_slices[motor], true);
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