cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2)
include(pico_sdk_import.cmake)

project(pico_ssd1306 C CXX ASM)

pico_sdk_init()
add_library(ssd1306_lib
    src/pico_ssd1306.c
    examples/sprites/sprites.c
)

target_include_directories(ssd1306_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/examples/sprites
)

target_link_libraries(ssd1306_lib PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_dma
)

file(GLOB EXAMPLE_FILES examples/*.c)

foreach(example ${EXAMPLE_FILES})
    get_filename_component(example_name ${example} NAME_WE)

    add_executable(${example_name} ${example})

    target_link_libraries(${example_name}
        pico_stdlib
        hardware_i2c
        hardware_dma
        ssd1306_lib
    )

    target_include_directories(${example_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/examples/sprites
    )

    pico_enable_stdio_usb(${example_name} 1)
    pico_enable_stdio_uart(${example_name} 0)

    pico_add_extra_outputs(${example_name})
endforeach()
