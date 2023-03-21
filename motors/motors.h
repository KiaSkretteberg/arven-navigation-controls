/*
 * motors.h
 * Controls the motors of the robot enabling forward/backward movement (and rotation)
 * 
 * motor controller datasheet: https://wiki.dfrobot.com/MD1.3_2A_Dual_Motor_Controller_SKU_DRI0002 
 *
 * Created: 2023-03-20
 * Author: Kia Skretteberg
 */
#define M1 2    // GPIO 2 [pin 4]
#define EN1 3   // GPIO 3 [pin 5]

#define M2 6    // GPIO 6 [pin 9]
#define EN2 7   // GPIO 7 [pin 10]
/*
#define M1 2    // GPIO 2
#define EN1 3   // GPIO 3
#define M1 2    // GPIO 2
#define EN1 3   // GPIO 3
#define M1 2    // GPIO 2
#define EN1 3   // GPIO 3
#define M1 2    // GPIO 2
#define EN1 3   // GPIO 3
*/

#define WHEEL_DIAMETER 0.065 // in m
#define MAX_RPM 140 // from datasheet for 36GP-555-27-EN motors, max rated torque speed
#define MOTOR_PERIOD 254 // max value as specified by datasheet for motor controllers: DRI0002

/** \brief Selector for specific motor:
 *  \ingroup motors
 */
typedef enum {
    Motor_FL = 1,
    Motor_FR = 2
} Motor;

/** \brief Selector for motor direction:
 *  \ingroup motors
 */
typedef enum {
    Motor_Forward = 0,
    Motor_Reverse = 1
} MotorDirection;

/** \brief Selector for motor direction:
 *  \ingroup motors
 */
typedef enum {
    Motor_PinType_Direction,
    Motor_PinType_Speed
} MotorPinType;

void motor_init_all(void);
uint motor_init(Motor motor);
void motor_stop(Motor motor);
void motor_forward(Motor motor, float speed);
void motor_reverse(Motor motor, float speed);