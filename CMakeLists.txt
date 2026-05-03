cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2)
include(pico_sdk_import.cmake)

project(pico_ssd1306 C CXX ASM)

pico_sdk_init()
add_library(ssd1306_lib
    src/pico_ssd1306.c
    assets/sprites/sprites.c
)

target_include_directories(ssd1306_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/assets
)

target_link_libraries(ssd1306_lib PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_dma
    hardware_adc
)

file(GLOB EXAMPLE_DIRS "examples/*")

foreach(dir ${EXAMPLE_DIRS})
    if(IS_DIRECTORY ${dir})
        
        get_filename_component(example_name ${dir} NAME)

        file(GLOB example_srcs "${dir}/*.c")

        if(example_srcs)
            add_executable(${example_name} ${example_srcs})

            target_link_libraries(${example_name}
                pico_stdlib
                hardware_i2c
                hardware_spi
                hardware_dma
                hardware_adc
                ssd1306_lib
            )

            target_include_directories(${example_name} PRIVATE
                ${PROJECT_SOURCE_DIR}/src
                ${PROJECT_SOURCE_DIR}/assets
            )

            pico_enable_stdio_usb(${example_name} 1)
            pico_enable_stdio_uart(${example_name} 0)

            pico_add_extra_outputs(${example_name})
        endif()
        
    endif()
endforeach()
