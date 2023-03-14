cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(arven C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(arven
    arven.c
    atmega.c atmega.h
    motors.c motors.h
)

pico_enable_stdio_usb(arven 1)
pico_enable_stdio_uart(arven 1)
pico_add_extra_outputs(arven)

target_link_libraries(arven pico_stdlib hardware_pwm)
