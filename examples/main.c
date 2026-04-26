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

#define WORKLOAD 1000000
static volatile float dummy = 1.0f;

static void cpu_work(){
    for(int i = 0; i < WORKLOAD; i++){
        dummy += (i ^ (int)dummy) * 0.000001f;
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
	ssd1306_update_display_dma(&disp1);
	sleep_ms(1000);
	ssd1306_clear_display(&disp1);
	sleep_ms(500);
	ssd1306_update_display_dma(&disp1);

	const sprite_t sprite_reel_1 = {
		.height = 32,
		.width = 32,
		.data = epd_bitmap_allArray[0], 
	};



		printf("creato oggetto sprite\n");
	
		printf("\n==== SSD1306 DMA vs BLOCKING BENCHMARK ====\n");

		// ======================================================
		// 1. BLOCKING DISPLAY TIME
		// ======================================================
		absolute_time_t t0 = get_absolute_time();

		ssd1306_update_display(&disp1);

		absolute_time_t t1 = get_absolute_time();

		int64_t blocking_time = absolute_time_diff_us(t0, t1);

		printf("[BLOCKING] display time: %lld us\n", blocking_time);


		sleep_ms(200);


		// ======================================================
		// 2. DMA SUBMIT TIME (quanto costa lanciare DMA)
		// ======================================================
		while(dma_channel_is_busy(disp1.dma_chan))
			tight_loop_contents();

		t0 = get_absolute_time();

		ssd1306_update_display_dma(&disp1);

		t1 = get_absolute_time();

		int64_t dma_submit_time = absolute_time_diff_us(t0, t1);

		printf("[DMA] submit time: %lld us (CPU quasi libera)\n", dma_submit_time);


		sleep_ms(200);


		// ======================================================
		// 3. CPU WORK DURANTE DMA (TEST VERO VANTAGGIO)
		// ======================================================
		ssd1306_update_display_dma(&disp1);

		t0 = get_absolute_time();

		cpu_work();   // qui DMA sta lavorando in background

		t1 = get_absolute_time();

		int64_t cpu_during_dma = absolute_time_diff_us(t0, t1);

		printf("[DMA] CPU work during transfer: %lld us\n", cpu_during_dma);


		// ======================================================
		// RISULTATO FINALE
		// ======================================================
		printf("\n==== RISULTATI ====\n");
		printf("Blocking  : %lld us (CPU FERMA)\n", blocking_time);
		printf("DMA submit: %lld us (overhead minimo)\n", dma_submit_time);
		printf("CPU+DMA   : %lld us (CPU continua a lavorare)\n", cpu_during_dma);

		printf("============================================\n");

		while(1){
			tight_loop_contents();
		}

	return 0;
}
