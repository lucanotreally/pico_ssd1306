#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

#include "pico_ssd1306.h" 
#include "sprites.h" 
#define OLED_ADDR 0x3C
#define OLED_BUF_SIZE 1024 //120*64 = 8192 bit o pixel, 8192/8 = 1024 byte di frame buffer


int main(){

	stdio_init_all();
	setup_default_uart();
	sleep_ms(1000);
	printf("codice runna\n");
	gpio_init(25);
	gpio_set_dir(25,GPIO_OUT);
	gpio_put(25,1);
	sleep_ms(1000);
	printf("inizializzo connessione seriale...\n");
	//sparato a 3.4MHz damn 750% del nominale	
	i2c_init(i2c0, 3400000);
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
	ssd1306_dma_init(&disp1);
	ssd1306_set_inversion_inverted(&disp1);
	sleep_ms(1000);
	ssd1306_set_inversion_normal(&disp1);
	printf("provo scroll\n");
	sleep_ms(1000);
	ssd1306_horizontal_scroll_init(&disp1,1,0,63,8);
	ssd1306_scroll_start(&disp1);
	sleep_ms(1000);
	ssd1306_scroll_stop(&disp1);
	sleep_ms(1000);
	printf("cancello pixel centrali\n");
	for (int i = 0;i<64;i++){
		ssd1306_clear_pixel(&disp1,63,i);
	}
	for (int i = 0;i<64;i++){
		ssd1306_clear_pixel(&disp1,32,i);
	}
	for (int i = 0;i<64;i++){
		ssd1306_clear_pixel(&disp1,96,i);
	}
	sleep_ms(1000);
	ssd1306_update_display(&disp1);
	sleep_ms(1000);
	//ssd1306_scroll_start(&disp1);
	
	ssd1306_clear_display(&disp1);
	ssd1306_update_display(&disp1);
	sleep_ms(1000);
	ssd1306_fill_display(&disp1);
	ssd1306_update_display(&disp1);
	/*	
	const sprite_t sprite_reel_1 = {
		.height = 32,
		.width = 32,
		.data = reel_frame_1
	};
	*/
	printf("creato oggetto sprite\n");
	sleep_ms(1000);


	
	ssd1306_clear_display(&disp1);
	ssd1306_update_display(&disp1);
	
	



	sleep_ms(1000);
	//ssd1306_draw_sprite_fast(&disp1,&sprite_reel_1,10,10);
	//1sleep_ms(1000);
	//ssd1306_update_display(&disp1);

	/*
	casa che scrolla
	int x = 63;
	int y = 2;
	ssd1306_draw_line(&disp1,x,y,x-20,y+10);
	ssd1306_draw_line(&disp1,x,y,x+20,y+10);
	ssd1306_draw_line(&disp1,x-20,y+10,x+20,y+10);
	ssd1306_draw_line(&disp1,x-20,y+10,x-20,y+35);
	ssd1306_draw_line(&disp1,x+20,y+10,x+20,y+35);
	ssd1306_draw_line(&disp1,x+20,y+35,x-20,y+35);
	ssd1306_update_display(&disp1);

	ssd1306_scroll_start(&disp1);
	*/
	static uint32_t last_draw_time =0; 
	const int fps = 120;
	const uint32_t frame_duration =1000/fps; 
//	const uint32_t frame_duration =0; 
	int current_frame = 0;
	uint32_t last_fps_time = to_ms_since_boot(get_absolute_time());
const float MAX_SPEED = 1.0f;
	bool is_playing = 1;
	int frames_drawn = 0;
	float reel_speed = 0.0f;      
	float reel_position = 0.0f;    
	float target_speed = 1.0f;     
	const float DAMPING = 0.97f;   
	const float ACCEL = 0.006f;    
	while(1) {
		uint32_t current_time = to_ms_since_boot(get_absolute_time());
		
		if(current_time>=16000){
			is_playing = 0;
		}
		
		if (current_time - last_draw_time >= frame_duration) {
			last_draw_time = current_time;

			if (is_playing) {
				if (reel_speed < MAX_SPEED) reel_speed += ACCEL;
			} else {
				reel_speed *= DAMPING;
				if (reel_speed < 0.01f) reel_speed = 0.0f;
			}

			reel_position += reel_speed;

			if (reel_position >= 9.0f) reel_position -= 9.0f;

			int current_frame_idx = (int)reel_position;

			sprite_t frame_sprite = {
				.data = epd_bitmap_allArray[current_frame_idx], 
				.width = 32,
				.height = 32
			};

			ssd1306_clear_display(&disp1);
			ssd1306_draw_sprite_fast(&disp1, &frame_sprite, 10, 10);
			//2nd reel
			ssd1306_draw_sprite_fast(&disp1, &frame_sprite, 86, 10); 

			ssd1306_update_display(&disp1);
			frames_drawn++;
		}

		//fps tracking
		if (current_time - last_fps_time >= 3000) {
			float actual_fps = (float)frames_drawn / 3.0f;
			printf("BENCHMARK: %.1f FPS \n", actual_fps);
			frames_drawn = 0;
			last_fps_time = current_time;
		}
	}
	return 0;
}
