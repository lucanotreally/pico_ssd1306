#include <math.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "pico/binary_info.h"

#include "pico_ssd1306.h" 
#include "sprites.h" 
#define OLED_ADDR 0x3C
#define OLED_BUF_SIZE 1024 //120*64 = 8192 bit o pixel, 8192/8 = 1024 byte di frame buffer

#define FRAMES 200
#define WORKLOAD 50000

volatile int dummy = 1;

// --------------------------------------------------
// CPU workload simulato
// --------------------------------------------------
static void cpu_work(){
	for(int i = 0; i < WORKLOAD; i++){
		dummy += (i ^ dummy) * 0.00001f;
	}
}

// --------------------------------------------------
// BLOCKING FPS
// --------------------------------------------------
static void benchmark_blocking(ssd1306_t *p, const sprite_t *s){

	absolute_time_t start = get_absolute_time();

	for(int i = 0; i < FRAMES; i++){

		ssd1306_clear_display(p);
		ssd1306_draw_sprite_fast(p, s, 10, 10);

		ssd1306_update_display(p);

		cpu_work();
	}

	int64_t t = absolute_time_diff_us(start, get_absolute_time());

	printf("[BLOCKING FPS] %.2f\n",
			(FRAMES * 1000000.0f) / t);
}

// --------------------------------------------------
// DMA FPS
// --------------------------------------------------
static void benchmark_dma(ssd1306_t *p, const sprite_t *s){

	absolute_time_t start = get_absolute_time();

	for(int i = 0; i < FRAMES; i++){

		ssd1306_clear_display(p);
		ssd1306_draw_sprite_fast(p, s, 10, 10);

		ssd1306_update_display_dma(p);

		cpu_work();
	}

	int64_t t = absolute_time_diff_us(start, get_absolute_time());

	printf("[DMA FPS] %.2f\n",
			(FRAMES * 1000000.0f) / t);
}

// --------------------------------------------------
// SINGLE UPDATE COST
// --------------------------------------------------
static void benchmark_blocking_time(ssd1306_t *p,const sprite_t *s){

	absolute_time_t start = get_absolute_time();

		ssd1306_clear_display(p);
	ssd1306_draw_sprite_fast(p, s, 10, 10);

	ssd1306_update_display(p);

	int64_t t = absolute_time_diff_us(start, get_absolute_time());

	printf("[BLOCKING UPDATE] %lld us\n", t);
}

// --------------------------------------------------
// CPU WORK DURING DMA
// --------------------------------------------------
static void benchmark_cpu_overlap(ssd1306_t *p, const sprite_t *s){

		ssd1306_clear_display(p);
	ssd1306_draw_sprite_fast(p, s, 10, 10);

	ssd1306_update_display_dma(p);

	absolute_time_t start = get_absolute_time();

	cpu_work();

	int64_t t = absolute_time_diff_us(start, get_absolute_time());

	printf("[CPU DURING DMA] %lld us\n", t);
}

// --------------------------------------------------
// JITTER TEST
// --------------------------------------------------
static void benchmark_jitter(ssd1306_t *p, const sprite_t *s){

	int64_t prev = 0;

	printf("[JITTER]\n");

	for(int i = 0; i < FRAMES; i++){

		absolute_time_t t0 = get_absolute_time();
		ssd1306_clear_display(p);
		ssd1306_draw_sprite_fast(p, s, 10, 10);

		ssd1306_update_display_dma(p);

		int64_t now = absolute_time_diff_us(t0, get_absolute_time());

		if(i > 0){
			printf("frame %d delta: %lld us\n", i, now - prev);
		}

		prev = now;
	}
}





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




	ssd1306_fill_display(&disp1);
	sleep_ms(500);
	ssd1306_update_display(&disp1);
	sleep_ms(1000);
	ssd1306_clear_display(&disp1);
	sleep_ms(500);
	ssd1306_update_display(&disp1);

	const sprite_t sprite = {
		.height = 32,
		.width = 32,
		.data = epd_bitmap_allArray[0], 
	};
	static uint32_t last_draw_time =0; 
	const int fps = 1000000;
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
	printf("creato oggetto sprite\n");

	printf("\n==== SSD1306 BENCHMARK SUITE ====\n\n");

	// FPS
	benchmark_blocking(&disp1, &sprite);
	benchmark_dma(&disp1, &sprite);

	sleep_ms(300);

	// blocking cost
	benchmark_blocking_time(&disp1, &sprite);

	sleep_ms(300);

	// CPU overlap
	benchmark_cpu_overlap(&disp1, &sprite);

	sleep_ms(300);

	// jitter
	benchmark_jitter(&disp1, &sprite);

	printf("\n==== DONE ====\n");
	
	
	sleep_ms(100);
	ssd1306_clear_display(&disp1);
	ssd1306_draw_sprite_fast(&disp1,&sprite,10,10);

	ssd1306_update_display_dma(&disp1);
	while(1){
		/*
		uint32_t current_time = to_ms_since_boot(get_absolute_time());
		if(current_time>=16000){
			is_playing = 0;
		}
*/
/*
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

			ssd1306_update_display_dma(&disp1);
			frames_drawn++;
		}

		//fps tracking
		if (current_time - last_fps_time >= 3000) {
			float actual_fps = (float)frames_drawn / 3.0f;
			printf("BENCHMARK: %.1f FPS \n", actual_fps);
			frames_drawn = 0;
			last_fps_time = current_time;
		
		}
	*/	
		tight_loop_contents();
	}

	return 0;
}
