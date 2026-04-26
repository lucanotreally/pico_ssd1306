#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/binary_info.h>

#include "pico_ssd1306.h"
#include "sprites/sprites.h"

#define FRAMES 200
#define WORKLOAD 50000

volatile int dummy = 1;

static void cpu_work(){
	for(int i = 0; i < WORKLOAD; i++){
		dummy += (i ^ dummy) * 0.00001f;
	}
}

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

	printf("[CPU WASTED TIME BC OF i2c SLOWNESS] %lld us\n", t);
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

	printf("[TIME IN WHICH CPU WAS ABLE TO DO OTHER TASKS AFTER SUBMITTING] %lld us\n", t);
}

// --------------------------------------------------
// JITTER TEST
// --------------------------------------------------
static void benchmark_jitter(ssd1306_t *p, const sprite_t *s){

    int64_t prev = 0;

    int64_t first = 0;
    int64_t sum = 0;
    int64_t min = INT64_MAX;
    int64_t max = 0;

    printf("[JITTER]\n");

    for(int i = 0; i < FRAMES; i++){

        absolute_time_t t0 = get_absolute_time();

        ssd1306_clear_display(p);
        ssd1306_draw_sprite_fast(p, s, 10, 10);
        ssd1306_update_display_dma(p);

        int64_t now = absolute_time_diff_us(t0, get_absolute_time());

        if(i == 0){
            first = now;
        } else {
            int64_t delta = now - prev;
            sum += delta;

            if(delta < min) min = delta;
            if(delta > max) max = delta;
        }

        prev = now;
    }

    int frames = FRAMES - 1;
    int64_t avg = (frames > 0) ? (sum / frames) : 0;

    printf("first frame: %lld us\n", first);
    printf("avg delta:    %lld us\n", avg);
    printf("min delta:    %lld us\n", min);
    printf("max delta:    %lld us\n", max);
}
int main(){
	
	stdio_init_all();
	setup_default_uart();
	printf("codice runna\n");
	gpio_init(25);
	gpio_set_dir(25,GPIO_OUT);
	gpio_put(25,1);
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
	ssd1306_dma_init(&disp1);

	ssd1306_set_inversion_inverted(&disp1);
	sleep_ms(1000);
	ssd1306_set_inversion_normal(&disp1);
	sleep_ms(1000);
	ssd1306_clear_display(&disp1);
	const sprite_t sprite = {
		.height = 32,
		.width = 32,
		.data = epd_bitmap_allArray[0]
	};
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
	while(1){
		tight_loop_contents();
	}
	return 0;
}
