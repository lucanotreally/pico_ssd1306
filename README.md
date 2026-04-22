# `pico_ssd1306`
A library to use the ssd1306 oled display with the RPi Pico/RP2040/RP2350. Written in C via the [pico-sdk](https://github.com/raspberrypi/pico-sdk) with efficiency in mind.
## Architecture overview
- **No use of dynamic memory**: everything is allocated at compile time
- **Double buffering**: there are two display buffers, so that one can be sent while the other one is already being modified to make up for serial communication bottleneck
- **Use of Direct Memory Access**: you can decide to use dma or the pico HAL to send the data to the display, dma usage allows you to bypass the serial communication bottleneck as well!!WIP!!
- **Kind of portable**: modifying this library for another mcu would not be that hard because of the way the raw serial connection is treated

