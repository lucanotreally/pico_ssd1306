#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

#include "pico_ssd1306.h" 
#define OLED_ADDR 0x3C
#define OLED_BUF_SIZE 1024 //120*64 = 8192 bit o pixel, 8192/8 = 1024 byte di frame buffer



int main(){

	stdio_init_all();
	setup_default_uart();
	sleep_ms(3000);
	printf("codice runna\n");
	gpio_init(25);
	gpio_set_dir(25,GPIO_OUT);
	gpio_put(25,1);
	sleep_ms(3000);
	printf("inizializzo connessione seriale...\n");
	i2c_init(i2c0, 400000);
	gpio_set_function(4, GPIO_FUNC_I2C);
	gpio_set_function(5, GPIO_FUNC_I2C);
	gpio_pull_up(4);
	gpio_pull_up(5);
	printf("inizializzato i2c\ncreo oggetto display...\n");

	ssd1306_t disp1;
	disp1.i2c_i = i2c0;
	disp1.external_vcc = 0;
	
	
	printf("creato display e assegnata istanza\ninizializzo schermo...\n");
	ssd1306_init(&disp1,128,64,OLED_ADDR,i2c0,0,0);
	sleep_ms(1000);
	ssd1306_set_inversion_inverted(&disp1);
	sleep_ms(3000);
	ssd1306_set_inversion_normal(&disp1);
	printf("provo scroll");
	sleep_ms(2000);
	ssd1306_horizontal_scroll_init(&disp1,1,0,63,8);
	ssd1306_scroll_start(&disp1);
	sleep_ms(3000);
	ssd1306_scroll_stop(&disp1);
	sleep_ms(100);
	printf("cancello pixel centrali");
	for (uint8_t i = 0;i<64;i++){
		ssd1306_clear_pixel(&disp1,63,i);
	}
	for (uint8_t i = 0;i<64;i++){
		ssd1306_clear_pixel(&disp1,32,i);
	}
	for (uint8_t i = 0;i<64;i++){
		ssd1306_clear_pixel(&disp1,96,i);
	}
	ssd1306_update_display(&disp1);
	sleep_ms(2000);
	//ssd1306_scroll_start(&disp1);
	ssd1306_clear_display(&disp1);
	ssd1306_update_display(&disp1);
	sleep_ms(1000);
	ssd1306_fill_display(&disp1);
	ssd1306_update_display(&disp1);
	sleep_ms(1000);

	ssd1306_clear_display(&disp1);
	ssd1306_draw_pixel(&disp1,0,0);
	ssd1306_draw_line(&disp1, 32,17,96,47);
	ssd1306_update_display(&disp1);
	ssd1306_scroll_start(&disp1);
	while(1){
		gpio_put(25,0);
		sleep_ms(1000);
		gpio_put(25,1);
		sleep_ms(1000);
	}
	return 0;
}
