#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include"pico_ssd1306.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "pico/binary_info.h"

//ssd1306 riconosce se byte mandato è di comando o un dato mandando due byte, primo di risconoscimento secondo di contenuto 0x00  = prossimo byte è comando, 0x40 = prossimo byte è dato



inline static void raw_write(i2c_inst_t *i2c, uint8_t addr, const uint8_t *data, size_t len, char *name) {
    switch(i2c_write_timeout_us(i2c, addr, data, len, false,100000)) {
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
} bool ssd1306_init(ssd1306_t *p,uint8_t width,uint8_t height,uint8_t address, i2c_inst_t *i2c_instance, bool x_flip, bool y_flip){ p->width=width; p->height=height;
    	p->pages=height/8;
	p->address=address;
	p->i2c_i=i2c_instance;
	p->full_buffer[0] = 0x40; //sets comm byte of final payload
	p->display_buffer = &p->full_buffer[1];
	


	uint8_t init_cmds[] = {
		//some commands are 1 byte and some are 2 bytes long, requiring a value, they are arranged so the single byte ones are alone on the row, otherwise the data is specified next to them
		//all param and value explanations in the header
		SET_DISP_OFF,
		SET_DISP_CLK_DIV, 0x80,
		SET_MUX_RATIO, height-1,
		SET_DISP_OFFSET, 0x00,
		SET_DISP_START_LINE,
		SET_CHARGE_PUMP,p->external_vcc?0x10:0x14,
		x_flip?FLIP_X_AXIS_ON:FLIP_X_AXIS_OFF,
		y_flip?FLIP_Y_AXIS_ON:FLIP_Y_AXIS_OFF,
		SET_COM_PIN_CFG, width>2*height?0x02:0x12,
		SET_CONTRAST, 0xFF,
		SET_PRECHARGE, p->external_vcc?0x22:0xF1,
		SET_VCOM_DESEL, 0x30,
		SET_DISPLAY_NORMAL,
		SET_INVERSION_NORMAL,
		SET_DISP_ON,
		SET_MEM_ADDR, 0x00
	};
	for(size_t i=0;i<sizeof(init_cmds);i++){
		ssd1306_send_cmd(p,init_cmds[i]);
	};

	return true;

}
/* sus name and usage
void ssd1306_deinit(ssd1306_t *p){
	ssd1306_send_cmd(p,SET_DISP_OFF);
	p->width = 0;
	p->height = 0;
	p->address = 0;
	p->pages = 0; 
}*/

void ssd1306_poweroff(ssd1306_t *p){
	ssd1306_send_cmd(p,SET_DISP_OFF);
}

void ssd1306_poweron(ssd1306_t *p){
	ssd1306_send_cmd(p,SET_DISP_ON);
}

void ssd1306_set_contrast(ssd1306_t *p,uint8_t contrast_percent){
	if (contrast_percent>100) contrast_percent = 100;
	uint8_t contrast_actual = (((uint32_t)contrast_percent * 255u)+50u)/100u;
	ssd1306_send_cmd(p,SET_CONTRAST);
	ssd1306_send_cmd(p,contrast_actual);
}

void ssd1306_set_inversion_normal(ssd1306_t *p){
	ssd1306_send_cmd(p, SET_INVERSION_NORMAL);
}

void ssd1306_set_inversion_inverted(ssd1306_t *p){
	ssd1306_send_cmd(p, SET_INVERSION_INVERTED);
}

void ssd1306_clear_display_buffer(ssd1306_t *p){
	memset(p->display_buffer, 0, (p->width*p->height)/8);
}

void ssd1306_horizontal_scroll_init(ssd1306_t *p, bool right, uint8_t start_line, uint8_t end_line, uint8_t speed){
	
	//display sees "pages", thought thinking in lines would be more natural
	uint8_t start_page = start_line / 8;
	uint8_t end_page   = (end_line > 0) ? (end_line - 1) / 8 : 0;
	//scroll speed bytes make little sense so i put them in a list so we can get a raising speed with range 1-8
	if (speed<1) speed = 1;
	if (speed>8) speed = 8;
	uint8_t speed_list[]={
		0x03,0x02,0x01,0x06,0x00,0x05,0x04,0x07};
//whole sequence that activates the scroll with its params
	uint8_t scroll_seq[]={
		[0] = 0x00,	//command byte
		[1] = 0x2E,	//scroll off
		[2] = (right)?0x26:0x27, //scroll direction right = 1
		[3] = 0x00,	//dummy byte
		[4] = start_page,	//starting line for scroll
		[5] = speed_list[speed-1],	//scroll speed
		[6] = end_page,		//
		[7] = 0x00,	//dummy byte
		[8] = 0xFF,	//dummy
	};
	raw_write(p->i2c_i,p->address,scroll_seq,sizeof(scroll_seq),"scroll_init");
}
void ssd1306_scroll_start(ssd1306_t *p){
	ssd1306_send_cmd(p, SET_SCROLL_ON);
	p->is_scrolling = true;
}
void ssd1306_scroll_stop(ssd1306_t *p){
	ssd1306_send_cmd(p,SET_SCROLL_OFF);
	p->is_scrolling = false;
}

void ssd1306_clear_pixel(ssd1306_t *p, uint8_t x, uint8_t y){
	if (x>=p->width || y>=p->height) return;
	uint16_t index = ((y>>3)*p->width) + x;
	p->display_buffer[index] &= ~(1<<(y&7));
}

void ssd1306_draw_pixel(ssd1306_t *p, uint8_t x, uint8_t y){
	if (x>=p->width || y>=p->height) return;
	uint16_t index = ((y>>3)*p->width) + x;
	p->display_buffer[index] |= (1<<(y&7));
}

void ssd1306_update_display(ssd1306_t *p){
	
	if (p->is_scrolling) ssd1306_scroll_stop(p);
	raw_write(p->i2c_i,p->address,p->full_buffer,1025,"ssd1306_update");
	if (p->is_scrolling) ssd1306_scroll_start(p);
}

void ssd1306_clear_display(ssd1306_t *p){
	memset(p->display_buffer, 0, 1024);
}

void ssd1306_fill_display(ssd1306_t *p){
	memset(p->display_buffer, 0xFF, 1024);
}

inline static void swap(int32_t *a, int32_t *b) {
    int32_t *t=a;
    *a=*b;
    *b=*t;
}
void ssd1306_draw_line(ssd1306_t *p, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    // 1. Calcoliamo le distanze assolute
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    
    // 2. Determiniamo la direzione del passo (1 o -1)
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    
    // 3. Variabile di decisione (errore)
    int err = dx + dy;
    int e2; 

    while (1) {
        // Disegniamo il pixel corrente
        ssd1306_draw_pixel(p, x1, y1);

        // Se siamo arrivati a destinazione, usciamo dal ciclo
        if (x1 == x2 && y1 == y2) break;

        e2 = 2 * err;

        // Decidiamo se muoverci lungo X
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }

        // Decidiamo se muoverci lungo Y
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}
