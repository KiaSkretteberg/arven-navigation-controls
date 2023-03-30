add_library(web web.c)

set(PICO_BOARD pico_w)

target_link_libraries(web
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_http
    pico_stdlib)