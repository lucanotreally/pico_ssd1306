#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include"pico_ssd1306.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "pico/binary_info.h"

//ssd1306 riconosce se byte mandato è di comando o un dato mandando due byte, primo di risconoscimento secondo di contenuto 0x00  = prossimo byte è comando, 0x40 = prossimo byte è dato



inline static void raw_write(i2c_inst_t *i2c, uint8_t addr, const uint8_t *data, size_t len, char *name) {
    switch(i2c_write_blocking(i2c, addr, data, len, false)) {
    case PICO_ERROR_GENERIC:
        printf("[%s] addr not acknowledged!\n", name);
        break;
    case PICO_ERROR_TIMEOUT:
        printf("[%s] timeout!\n", name);
        break;
    default:
        //printf("[%s] wrote successfully %lu bytes!\n", name, len);
        break;
    }
}

inline static void ssd1306_send_cmd(ssd1306_t *p, uint8_t val){
	uint8_t cmd_string[2] = {0x00,val};
	raw_write(p->i2c_i,p->address,cmd_string,2,"ssd1306_send_cmd");
}

void ssd1306_init(ssd1306_t *p,uint8_t width,uint8_t height,uint8_t address, i2c_inst_t *i2c_istance){
	p->width=width;
	p->height=height;
    	p->pages=height/8;
	p->address=address;
	p->i2c_i=i2c_instance;
	
	uint8_t init_cmds[] = {
		//some commands are 1 byte and some are 2 bytes long, requiring a value, they are arranged so the single byte ones are alone on the row, otherwise the data is specified next to them
		//all param and value explanations in the header
		SET_DISP_OFF,
		SET_DISP_CLK_DIV, 0x80,
		SET_MUX_RATIO, height-1,
		SET_DISP_OFFSET, 0x00,
		SET_DISP_START_LINE,
		SET_CHARGE_PUMP,p->external_vcc?0x10:0x14,
		SET_SEG_REMAP,
		SET_COM_OUT_DIR, width>2*height?0x02:0x12,
		SET_CONTRAST, 0xFF,
		SET_PRECHARGE, p->external_vcc?0x22:0xF1,
		SET_VCOM_DESEL 0x30,
		SET_DISPLAY_NORMAL,
		SET_NORM_INV,
		SET_DISP_ON,
		SET_MEM_ADDR, 0x00
	}
	for(size_t i=0;i<sizeof(init_cmds);i++){
		ssd1306_send_cmd(p,init_cmds[i]);
	}

	return true
}

/* sus name and usage
inline void ssd1306_deinit(ssd1306_t *p){
	ssd1306_send_cmd(p,SET_DISP_OFF);
	p->width = 0;
	p->height = 0;
	p->address = 0;
	p->pages = 0; 
}*/

inline void ssd1306_poweroff(ssd1306_t *p){
	ssd1306_send_cmd(p,SET_DISP_OFF);
}

inline void ssd1306_poweroff(ssd1306_t *p){
	ssd1306_send_cmd(p,SET_DISP_ON);
}

void oled_update(uint8_t *buffer){
	// 1. Diciamo all'OLED in quale area dello schermo vogliamo scrivere (TUTTO)
	oled_send_cmd(0x21); // Set Column Address (colonne)
	oled_send_cmd(0x00); // Colonna di partenza: 0
	oled_send_cmd(0x7F); // Colonna finale: 127

	oled_send_cmd(0x22); // Set Page Address (righe/pagine)
	oled_send_cmd(0x00); // Pagina di partenza: 0
	oled_send_cmd(0x07); // Pagina finale: 7 (8 pagine totali da 8 pixel l'una = 64 pixel)
	
	uint8_t temp_buffer[OLED_BUF_SIZE + 1]; //1 byte piu grande perche contiene anche 0x40 che dice al display che si tratta di dato
	temp_buffer[0] = 0x40;
	//ora dopo lo 0x40 metto i DATI quindi in questo caso il mio buffer
	for (int i = 0; i < OLED_BUF_SIZE; i++) {
        temp_buffer[i + 1] = buffer[i];
   	}
	i2c_write_blocking(i2c0,OLED_ADDR,temp_buffer,OLED_BUF_SIZE+1,false);
}


int main(){
	stdio_init_all();
	setup_default_uart();
	gpio_init(25);
	gpio_set_dir(25,GPIO_OUT);
	gpio_put(25,1);
	i2c_init(i2c0, 400*1000);
	gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN,GPIO_FUNC_I2C);
	gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN,GPIO_FUNC_I2C);
	gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
	gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
	//sleep_ms(3000);
	printf("inizializzato i2c\n");
	oled_init();
	//sleep_ms(2000);
	printf("puliamo schermo\n");
	uint8_t buffer_vuoto[OLED_BUF_SIZE] = {0};
	uint8_t buffer_pieno[OLED_BUF_SIZE];
	memset(buffer_pieno,0xFF,OLED_BUF_SIZE);
	oled_update(buffer_vuoto);
	//sleep_ms(2000);
	printf("riempiamo display");
	oled_update(buffer_pieno);
	while(1){
		printf("messaggio\n");
		sleep_ms(2000);
}	
	
	return 0;
}

