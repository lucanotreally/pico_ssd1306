#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico_ssd1306.h"

#define OLED_I2C_ADDRESS 0x3C
//the nominal value should be 400kHz, my unit worked great all the way up to 3.4MHz
#define I2C_FREQUENCY 3400000 

/*	
	Example of a use case for the library: we sample a voltage given from a pot via the pico
	ADC and use that value to move the level on a progress bar, which also changes the display
	contrast (on my unit the effect is not that noticeable). The ADC is not that great, so we use
	only the first 7 bit of resolution (out of 12) to filter out some of the noise and nonsense.
	And then have to add a variance THRESHOLD so it does not fluctuate and move the bar around, yeah...
*/


//under 2 the electrical noise makes reading unstable
//gives 64 read per turn on a pot, enough for this demo
#define THRESHOLD 2




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
	



	ssd1306_init_i2c(&disp1,128,64,i2c0,OLED_I2C_ADDRESS,0,0);
	ssd1306_dma_init(&disp1);
	
	
	//init the adc, select the pin and adc#
	adc_init();
	adc_gpio_init(26);
	adc_select_input(0);

	//constant to convert the ADC reading to voltage	
	const float scaling_constant = 3.3f / (1 << 7);
	//temp value to compare the level random value in this example
	uint8_t temp = 37;


	//draw something on the screen before the loop so we have something before the adc reading varies
	ssd1306_clear_display(&disp1);
	ssd1306_draw_fill_rect_fast(&disp1,10,10,50,15);
	ssd1306_update_display(&disp1);

	while (1){


		//here we remove the 5LSBs from the reading as said earlier

		uint8_t result = adc_read() >> 5;

		//updates the screen only if a difference in value is detected

		if (abs((int)result-(int)temp) >=THRESHOLD){

			
			uint8_t percentage = (result * 100) / 128;
			
			printf("Value: %d \t Voltage: %f V \t Perc: %d \n",result,result*scaling_constant,percentage);
			
			
			ssd1306_clear_display(&disp1);
		
		//draws the outer bar rectangle every time, which dimensions do not change

			ssd1306_draw_rect_fast(&disp1,9,9,102,17);

		//this is our progressing bar
		
			ssd1306_draw_fill_rect_fast(&disp1,10,10,percentage,15);
		
			ssd1306_update_display_dma(&disp1);
			
			ssd1306_set_contrast(&disp1,percentage);
			
			temp = result;
			
			uint8_t temp_perc = (temp * 100) / 128;
		}
	}


	return 0;
}
