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
}

bool ssd1306_init(ssd1306_t *p,uint8_t width,uint8_t height,uint8_t address, i2c_inst_t *i2c_instance){
	p->width=width;
	p->height=height;
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
		SET_SEG_REMAP,
		SET_COM_OUT_DIR, width>2*height?0x02:0x12,
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

inline void ssd1306_poweron(ssd1306_t *p){
	ssd1306_send_cmd(p,SET_DISP_ON);
}

inline void ssd1306_set_contrast(ssd1306_t *p,uint8_t contrast_percent){
	if (contrast_percent>100) contrast_percent = 100;
	uint8_t contrast_actual = (((uint32_t)contrast_percent * 255u)+50u)/100u;
	ssd1306_send_cmd(p,SET_CONTRAST);
	ssd1306_send_cmd(p,contrast_actual);
	
	
}

inline void ssd1306_set_inversion_normal(ssd1306_t *p){
	ssd1306_send_cmd(p, SET_INVERSION_NORMAL);
}

inline void ssd1306_set_inversion_inverted(ssd1306_t *p){
	ssd1306_send_cmd(p, SET_INVERSION_INVERTED);
}

inline void ssd1306_clear_display_buffer(ssd1306_t *p){
	memset(p->display_buffer, 0, (p->width*p->height)/8);
}





