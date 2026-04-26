cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico2)
include(pico_sdk_import.cmake)

project(ssd1306 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})


pico_sdk_init()

#nome del file eseguibile generato
add_executable(ssd1306
	examples/main.c
	src/pico_ssd1306.c
	src/sprites/sprites.c
)

target_compile_definitions(ssd1306
	PRIVATE
	SSD1306_USE_DMA=1
)

target_include_directories(ssd1306 PRIVATE
    src
    src/sprites
    src/fonts
)
#dichiaro che c'è un programma PIO, chiamato new_scan.pio, che voglio
#buildare come HEADER per il mio file c, per essere buildato
#nell'eseguibile scan_pio(come detto prima)
pico_generate_pio_header(ssd1306 ${CMAKE_CURRENT_LIST_DIR}/src/ssd1306.pio)


#listo i file source per il programma scan_pio

target_sources(ssd1306 PRIVATE examples/main.c)

#includo le librerie dei vari API per poter usare le funzioni specifiche 
#nel programma C
target_link_libraries(ssd1306 PRIVATE
	pico_stdlib
	hardware_pio
	hardware_clocks
	hardware_adc
	hardware_pwm
	hardware_i2c	
	hardware_dma
	hardware_sync
	)

pico_enable_stdio_usb(ssd1306 1)
pico_enable_stdio_uart(ssd1306 0)
#il compilatore returna solo un eseguibile, gli dico di farmi anche un .uf2
#(usb to file) per drag and drop nel pico
pico_add_extra_outputs(ssd1306)
