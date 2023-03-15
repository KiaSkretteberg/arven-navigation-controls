add_library(motors motors.c)

target_link_libraries(motors
    pico_stdlib 
    hardware_pwm)