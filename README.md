#`pico_ssd1306`
A library to use the ssd1306 oled display with the RPi Pico/RP2040/RP2350. Written in C via the [pico-sdk](https://github.com/raspberrypi/pico-sdk) with efficiency in mind.
## Architecture overview
- **No use of dynamic memory**: everything is allocated at compile time
- **Double buffering**: the display buffer is actually made up of two, so that one can be sent while the other one is already being modified to make up for serial communication bottleneck
- **Use of Direct Memory Access**: so that the cpu is free to do other task once the payload is pointed to the DMA
- **Kind of portable**: modifying this library for another mcu would not be that hard because of the way the raw serial connection is treated

