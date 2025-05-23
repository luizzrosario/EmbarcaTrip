cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(EmbarcaTrip C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(EmbarcaTrip EmbarcaTrip.c src/ssd1306.c)

pico_set_program_name(EmbarcaTrip "EmbarcaTrip")
pico_set_program_version(EmbarcaTrip "0.1")

# Generate PIO header
pico_generate_pio_header(EmbarcaTrip ${CMAKE_CURRENT_LIST_DIR}/utils/ws2812.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(EmbarcaTrip 1)
pico_enable_stdio_usb(EmbarcaTrip 1)

# Add the standard library to the build
target_link_libraries(EmbarcaTrip
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(EmbarcaTrip PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(EmbarcaTrip 
        hardware_spi
        hardware_i2c
        hardware_pio
        hardware_pwm
        hardware_adc
        hardware_clocks
        )

pico_add_extra_outputs(EmbarcaTrip)

