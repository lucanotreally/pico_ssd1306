
#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico_ssd1306.h"
//the nominal value should be 400kHz, my unit worked great all the way up to 3.4MHz
#define I2C_FREQUENCY 3400000 
#define OLED_I2C_ADDRESS 0x3C
#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128

/*	
	This program draws lines on the screen based on sample saved in a buffer. That buffer is filled with values
	taken from the ADC which is connected to a microphone breakout board, the MAX9814. Calling this oscilloscope
	is an exaggeration, it is a fun wave visualizer with a fixed time base.
*/

/*
	The MAX9814 bb states the module outputs 2vpp max centered at 1.25V, so:
	-minimum voltage (silence) : voltage_min = 1.25-(vpp/2)= 0.25V
	-maximum voltage : voltage_max = 1.25+(vpp/2)= 2.25V
	now we gotta convert this voltages in values in perspective of the 12bit ADC:
	value_min = 4096/3.3 * voltage_min = 310
	value_max = 4096/3.3 * voltage_max = 2793
	value_center = 4096/3.3 * 1.25 = 1552
*/


#define ADC_MAX 2793
#define ADC_MIN	310
#define ADC_CENTER 1552

//this decides how "streched" your wave will look like
#define TIMEBASE_US 20




void i2c_setup(){

	i2c_init(i2c0, I2C_FREQUENCY);
	gpio_set_function(4, GPIO_FUNC_I2C);
	gpio_set_function(5, GPIO_FUNC_I2C);
	gpio_pull_up(4);
	gpio_pull_up(5);

}




int main(){
	stdio_init_all();

	//wrapped the basic i2c calls in a function

	i2c_setup();

	//init the 25th pin on the pico which is the onboard led, to debug	

	gpio_init(25);
	gpio_set_dir(25,GPIO_OUT);
	gpio_put(25,1);
	
	//create display object called disp1
	//we could assign other values to it 
	//in this example i just passed them as values later in the init
	ssd1306_t disp1 = {
		.external_vcc = 0
	};
	



	ssd1306_init_i2c(&disp1,SCREEN_WIDTH,SCREEN_HEIGHT,i2c0,OLED_I2C_ADDRESS,0,0);
	ssd1306_dma_init(&disp1);
	
	
	//init the adc, select the pin and adc#
	adc_init();
	adc_gpio_init(26);
	adc_select_input(0);

	uint8_t x_tracking = 0;

	ssd1306_clear_display(&disp1);
	uint16_t sample_buffer[SCREEN_WIDTH];	

	//main loop
	while (1){

		
		//trigger event, so the phase on the screen remains costant
		while(adc_read() > ADC_CENTER) { tight_loop_contents(); }
		while(adc_read() < ADC_CENTER) { tight_loop_contents(); }
		

		//here we sample, we take 128 values from the adc
		for (int i = 0; i < SCREEN_WIDTH; i++) {
			sample_buffer[i] = adc_read();
			sleep_us(TIMEBASE_US); 
		}
		

		ssd1306_clear_display(&disp1);


		for (int x = 0; x < SCREEN_WIDTH - 1; x++) {

			int raw1 = sample_buffer[x];
			int raw2 = sample_buffer[x+1];

			if (raw1 < ADC_MIN) raw1 = ADC_MIN;
			if (raw1 > ADC_MAX) raw1 = ADC_MAX;
			if (raw2 < ADC_MIN) raw2 = ADC_MIN;
			if (raw2 > ADC_MAX) raw2 = ADC_MAX;

			//here we map the 12bit adc values to 0-63 range for the display
			uint8_t y1 = (raw1 - ADC_MIN) * (SCREEN_HEIGHT - 1) / (ADC_MAX - ADC_MIN);
			uint8_t y2 = (raw2 - ADC_MIN) * (SCREEN_HEIGHT - 1) / (ADC_MAX - ADC_MIN);

			ssd1306_draw_line(&disp1, x, y1, x + 1, y2);
		}

		ssd1306_update_display(&disp1);

		//delay for visibility
		sleep_ms(45);


	}


	return 0;
}
