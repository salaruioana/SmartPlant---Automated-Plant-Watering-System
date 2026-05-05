# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(smart_plant C CXX ASM)

pico_sdk_init()


add_executable(smart_plant
    smart_plant.c
    oled/ssd1306.c
)


pico_set_program_name(smart_plant "smart_plant")
pico_set_program_version(smart_plant "0.1")

# USB pentru printf/debug, UART dezactivat pe stdio
pico_enable_stdio_uart(smart_plant 0)
pico_enable_stdio_usb(smart_plant 1)

# Toate bibliotecile necesare - UN SINGUR BLOC
target_link_libraries(smart_plant 
        pico_stdlib
        hardware_i2c
        hardware_dma
        hardware_timer
        hardware_pwm
        hardware_adc
)

target_include_directories(smart_plant PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/oled
)



pico_add_extra_outputs(smart_plant)