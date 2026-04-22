#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include"pico_ssd1306.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "pico/binary_info.h"
#include "hardware/dma.h"
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


bool ssd1306_init(ssd1306_t *p,uint8_t width,uint8_t height,uint8_t address, i2c_inst_t *i2c_instance, bool x_flip, bool y_flip){ p->width=width; p->height=height;
    	p->pages=height/8;
	p->address=address;
	p->i2c_i=i2c_instance;
	p->active_buffer = 0;
	p->full_buffer[0][0] = 0x40; //sets comm byte of final payload
	p->full_buffer[1][0] = 0x40; //same
	p->display_buffer = &p->full_buffer[p->active_buffer][1];
	


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

void ssd1306_clear_display(ssd1306_t *p){
	memset(p->display_buffer, 0, (p->width*p->height)/8);
}

void ssd1306_fill_display(ssd1306_t *p){
	memset(p->display_buffer, 0xFF, (p->width*p->height)/8);
}

void ssd1306_horizontal_scroll_init(ssd1306_t *p, bool right, uint8_t start_page, uint8_t end_page, uint8_t speed){
	
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

void ssd1306_clear_pixel(ssd1306_t *p, int x, int y){
	if (x>=p->width || y>=p->height || x < 0 || y < 0) return;
	uint16_t index = ((y>>3)*p->width) + x;
	p->display_buffer[index] &= ~(1<<(y&7));
}

void ssd1306_draw_pixel(ssd1306_t *p, int x, int y){
	if (x>=p->width || y>=p->height || x < 0 || y < 0) return;
	uint16_t index = ((y>>3)*p->width) + x;
	p->display_buffer[index] |= (1<<(y&7));
}

void ssd1306_update_display(ssd1306_t *p){
	
	if (p->is_scrolling) ssd1306_scroll_stop(p);
	raw_write(p->i2c_i,p->address,p->full_buffer[p->active_buffer],1025,"ssd1306_update");
	if (p->is_scrolling) ssd1306_scroll_start(p);
}

void ssd1306_draw_sprite_slow(ssd1306_t *p, const sprite_t *s, int x, int y) {
    for (int iy = 0; iy < s->height; iy++) {
        for (int ix = 0; ix < s->width; ix++) {
		
            int byte_index = (iy / 8) * s->width + ix;
            uint8_t byte = s->data[byte_index];

            bool is_pixel_on = byte & (1 << (iy & 7));

            if (is_pixel_on) {
                ssd1306_draw_pixel(p, x + ix, y + iy);
            }
        }
    }
}

void ssd1306_draw_sprite_fast(ssd1306_t *p, const sprite_t *s, int x, int y){
	//checks if whole sprite is out of bounds, if so just ignore it
	if(x>=p->width||y>=p->height||x+(s->width)<=0||y+(s->height)<=0) return;
	//now we "cut" the sprite to write in the buffer only the visible part of it
	int x_start = (x < 0) ? -x : 0;
	int x_end = ((x+s->width)>=p->width) ? (p->width)-x : s->width;
	int y_start = (y < 0) ? -y : 0;
	int y_end = ((y+s->height)>=p->height) ? (p->height)-y : s->height;
	//tells us how many lines are empty(at the top) within a page
	//ex @y = 3, we are at PAGE0, but first 3 lines are empty
	int shift = y % 8;
	//correction block, bc C gives back a negative remainder for a negative input coordinate
	if (shift < 0) shift += 8;
	//finds the first page into which the sprite will be drawn
	int first_page = y/8;
	//correction for negative again, decrements first_page if its <0 but > -8 so that it is not drawn on page 0
	//cause of that y/8, ex: @y=-5, -5/8 = 0, so the sprite would start in p0 even if it is obviously not
	//the case
	if (y < 0 && (y % 8 != 0)) first_page--;

	//this for goes through the SPRITE pages(1B tall), good bc y_start is cut beforehand so 
	//this does not loop over sprite region which are out of visible screen
	for (int sy = (y_start / 8); sy <= (y_end - 1) / 8; sy++) {
		//adds the current sprite page to first page
		int target_page = first_page + sy;
		:
		if (target_page < 0 || target_page >= 8) continue;
		//now we loop through the bytes whitin the page
		for (int sx = x_start; sx < x_end; sx++) {
			//copies the byte at the cur poition
			uint8_t sprite_byte = s->data[sy * s->width + sx];
			//adds sd (curr byte within page) to x (pos coordinate)
			//so we know the final coordinate on the screen
			int target_x = x + sx;
			//index to place ^ coordinate converted with the page logic (same as draw pixel basically)
			int idx = (target_page * 128) + target_x;
			//we sum (|=) the byte in, but we shift (up) bc we have to account for blank lines
			//without <<shift everything would be glued at the top of the page
			p->display_buffer[idx] |= (sprite_byte << shift);
			//this takes care of the remaining bit in the next page
			//>>(8-shift) makes sure we leave blank lines (pixels per byte) at the bottom 
			if (shift > 0 && (target_page + 1) < 8) {
				p->display_buffer[idx + 128] |= (sprite_byte >> (8 - shift));
			}
        }
    }

}

void ssd1306_draw_line(ssd1306_t *p, int x1, int y1,int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    
    int err = dx + dy;
    int e2; 

    while (1) {
        ssd1306_draw_pixel(p, x1, y1);

        if (x1 == x2 && y1 == y2) break;

        e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}

bool ssd1306_dma_init(ssd1306_t *p){
	p->dma_chan = dma_claim_unused_channel(false);
	if (p->dma_chan < 0) return false; //pico couldnt find free dma channel
	dma_channel_config c = dma_channel_get_default_config(p->dma_chan);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_8);	//send 1 byte at a time
	channel_config_set_read_increment(&c,true);
	channel_config_set_write_increment(&c,false);
	channel_config_set_dreq(&c, i2c_get_dreq(p->i2c_i,true)); //synchronizing dma bus speed with i2c instance
	dma_channel_configure(
		p->dma_chan,
		&c,
		&i2c_get_hw(p->i2c_i)->data_cmd,	//destination
		p->full_buffer[p->active_buffer],	//source
		1025,	//size of payload sent
		false	//not starting transfer
	);
	return true;
}
void ssd1306_update_display_dma(ssd1306_t *p){
	if(p->dma_chan < 0){
		ssd1306_update_display(p);
		return;
	}
	while(dma_channel_is_busy(p->dma_chan)){
		tight_loop_contents();
	}
	uint8_t *payload_buffer = p->full_buffer[p->active_buffer];
	p->active_buffer = 1 - p->active_buffer;
	p->display_buffer = &p->full_buffer[p->active_buffer][1];
	//here i tell dma where to send the data, does this every update bc i2c bus could be busy and i the tar(get) could be changed by other functions
	i2c_get_hw(p->i2c_i)->enable = 0;
	i2c_get_hw(p->i2c_i)->tar = p->address;
	i2c_get_hw(p->i2c_i)->enable = 1;

	dma_channel_set_read_addr(p->dma_chan,payload_buffer,false);
	dma_channel_set_trans_count(p->dma_chan, 1025, true);

}
