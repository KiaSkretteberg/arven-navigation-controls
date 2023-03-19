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
    Motor_PinType_Speed,
    Motor_PinType_All
} MotorPinType;

void motor_init_all(void);
uint motor_init(Motor motor);
void motor_stop(Motor motor);
void motor_forward(Motor motor, float speed);
void motor_reverse(Motor motor, float speed);