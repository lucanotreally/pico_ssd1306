# `pico_ssd1306`
A library to use the ssd1306 oled display with the RPi Pico/RP2040/RP2350. Written in C via the [pico-sdk](https://github.com/raspberrypi/pico-sdk) with efficiency in mind.
## Architecture overview
- **No use of dynamic memory**: everything is allocated at compile time
- **Double buffering**: there are two display buffers, so that one can be sent while the other one is already being modified, this avoids tearing and lowers CPU blocking time
- **Use of Direct Memory Access**: you can decide to use dma or the pico HAL to send the data to the display, DMA does not increase throughput, but lowers CPU blocking during display updates by a [lot](#dma-benchmark)
- **Kind of portable**: modifying this library for another mcu would not be that hard because of the way the raw serial connection is treated

