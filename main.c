#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

#define OLED_ADDR 0x3C
#define OLED_BUF_SIZE 1024 //120*64 = 8192 bit o pixel, 8192/8 = 1024 byte di frame buffer

int waitime = 2;
//ssd1306 riconosce se byte mandato è di comando o un dato mandando due byte, primo di risconoscimento secondo di contenuto 0x00  = prossimo byte è comando, 0x40 = prossimo byte è dato
void oled_send_cmd(uint8_t cmd){
	uint8_t buf[2]; //array di due byte da mandare
	buf[0] = 0x00;//imposto zero in quanto questa funzione manda solo comandi
	buf[1] = cmd; //comando come argomento funzione
	i2c_write_blocking(i2c0, OLED_ADDR, buf, 2 ,false);

}

void oled_init() {
    uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Imposta il clock
        0xA8, 0x3F, // Multiplex ratio (per 64 righe)
        0xD3, 0x00, // Display offset a 0
        0x40,       // Start line a 0
        0x8D, 0x14, // ATTIVA LA CHARGE PUMP (Alta tensione per i LED)
        0x20, 0x00, // Memory addressing mode: Horizontal
        0xA1,       // Capovolgi orizzontalmente
        0xC8,       // Capovolgi verticalmente
        0xDA, 0x12, // Configurazione pin COM
        0x81, 0xCF, // Contrasto (0-255)
        0xD9, 0xF1, // Pre-charge period
        0xDB, 0x40, // VCOMH deselect level
        0xA4,       // Segui la RAM
        
        // LA MODIFICA È QUI: 0xA7 invece di 0xA6
        0xA6,       // Invert Display (0=Bianco, 1=Nero)
        
        0xAF        // DISPLAY ON!
    };

    int num_cmds = sizeof(init_cmds) / sizeof(init_cmds[0]);
    for (int i = 0; i < num_cmds; i++) {
        oled_send_cmd(init_cmds[i]);
    }
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

