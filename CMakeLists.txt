cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(arven C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(arven
    arven.c
)

pico_enable_stdio_usb(arven 1)
pico_enable_stdio_uart(arven 1)
pico_add_extra_outputs(arven)

# add our custom libraries
add_subdirectory(atmega)
add_subdirectory(dwm1001)
add_subdirectory(encoders)
add_subdirectory(ir)
add_subdirectory(motors)
add_subdirectory(ultrasonic)
add_subdirectory(weight)
add_subdirectory(web)

target_link_libraries(arven 
    atmega
    dwm1001
    encoders
    ir
    motors
    ultrasonic
    weight
    web
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_http
    pico_stdlib 
    hardware_pwm)

target_include_directories(arven PRIVATE
    "${PROJECT_SOURCE_DIR}/atmega"
    "${PROJECT_SOURCE_DIR}/dwm1001"
    "${PROJECT_SOURCE_DIR}/encoders"
    "${PROJECT_SOURCE_DIR}/ir"
    "${PROJECT_SOURCE_DIR}/motors"
    "${PROJECT_SOURCE_DIR}/ultrasonic"
    "${PROJECT_SOURCE_DIR}/weight"
    "${PROJECT_SOURCE_DIR}/web")
