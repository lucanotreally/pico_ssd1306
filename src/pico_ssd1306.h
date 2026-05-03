#ifndef PICO_SSD1306_H
#define PICO_SSD1306_H 

#include <stdint.h>
#include "hardware/i2c.h"
#include "hardware/spi.h"
#define OLED_ADDR 0x3C //standard sd1306 i2c address
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_BUF_SIZE 1024 //(128*64)/8 = 1024, divide by 8 bc it's in byte



typedef struct {
	//serial comm instance pointer, declared as void
	//so we can assign a i2c or spi instance as needed
	void *hw_inst;

	// display buffer that points to full_buffer[active_buffer][1], done so i can send only 1 array when uploading screen each time, reducing serial comm load
	uint8_t *display_buffer; 
	uint8_t full_buffer[2][1025];
	//union used here to allocate just the needed byte
	//using i2c we dont need the other 3 params
	union{
		uint8_t address;
		struct{
			uint8_t dc;
			uint8_t cs;
			uint8_t res;
		} spi;

	} hw_config;
	uint8_t width;	
	uint8_t height; 	
	uint8_t pages;		//stores pages of display (calculated on initialization)*
	int8_t dma_chan;

	bool active_buffer : 1;
	bool external_vcc : 1; 	// whether display uses external vcc 
	bool protocol : 1;	//0 = i2c, 1 = spi
	bool is_scrolling : 1; 
} ssd1306_t;

typedef enum{
	SSD1306_PROTOCOL_I2C = 0,
	SSD1306_PROTOCOL_SPI = 1
} ssd1306_protocol_t;


typedef enum {

	SET_DISP_OFF = 0xAE, //0xAE turns display off, low pwr sleep mode, 0xAF turns display on
	SET_DISP_ON = 0xAF,
	SET_CONTRAST = 0x81,//set contrast 0-255 -> 0x00-0xFF
	SET_INVERSION_NORMAL= 0xA6,//normal mode, 0xA7 inverts bits(1=off, 0=on)
	SET_INVERSION_INVERTED = 0xA7,
	SET_DISPLAY_NORMAL= 0xA4,//show what is in ram
	SET_ENTIRE_ON = 0xA5, //ignore ram, all pixels white

	SET_MEM_ADDR = 0x20,//sending 0x00 makes horizontal(normal) mode 0x01 = vertical 0x02 = page, read documentation
	SET_COL_ADDR = 0x21,//only used for vertical and page mode
	SET_PAGE_ADDR = 0x22,//same

	FLIP_X_AXIS_OFF = 0xA0,
	FLIP_X_AXIS_ON = 0xA1,
	FLIP_Y_AXIS_OFF = 0xC0,
	FLIP_Y_AXIS_ON = 0xC8,
	SET_MUX_RATIO = 0xA8,//#of vertical pixels, ssd1306 comes in 64 or 32, for 64 we send 0x3F(63) and 0x1F(31)
	SET_DISP_START_LINE = 0x40,//hardware way of schangin y offest of screen
	SET_DISP_OFFSET = 0xD3,//moves screen down to calibrate
	SET_COM_PIN_CFG = 0xDA,//0x12 for 128x64,0x02 for 128x32 version

	SET_CHARGE_PUMP = 0x8D,//needed to drive leds
	SET_DISP_CLK_DIV = 0xD5,//display frequency, 0xF0 is max 0x80 typical
	SET_PRECHARGE = 0xD9,//sharpness of pixels,0xF1 is good
	SET_VCOM_DESEL = 0xDB,//sets vref for off pixels, 0x30 typical
	SET_SCROLL_OFF = 0x2E,
	SET_SCROLL_ON = 0x2F
} ssd1306_command_t ;




typedef struct{
	uint8_t width;
	uint8_t height;
	const uint8_t *data;

} sprite_t;




/**
 * @brief Initializes the ssd1306 display using SPI, allows communication via the pico HAL and DMA.
 *
 *
 *
 *
 * @param p Pointer to the display object to init.
 * @param width Display width in pixel.
 * @param height Display height in pixel.
 * @param spi_instance Pointer to the pico spi instance (spi0 or spi1).
 * @param cs_pin GPIO pin used for the chip select signal. 
 * @param dc_pin GPIO pin used for the data/command signal. 
 * @param res_pin GPIO pin used for the reset signal. Set to 255 if not used.
 * @param x_flip If true, flips the display orientation horizontally.
 * @param y_flip If true, flips the display orientation vertically.
 *
 * @return true if the init went smoothly false otherwise.
 *
 * @warning This init function contains 2 sleep calls, needed for the spi reset pin.
 */
bool ssd1306_init_spi(ssd1306_t *p, uint8_t width, uint8_t height, spi_inst_t *spi_instance, uint8_t cs_pin, uint8_t dc_pin, uint8_t res_pin, bool x_flip, bool y_flip);


/**
 * @brief Initializes the ssd1306 display using I2C, allows communication via the pico HAL and DMA.
 *
 *
 *
 *
 * @param p Pointer to the display object to init.
 * @param width Display width in pixel.
 * @param height Display height in pixel.
 * @param i2c_instance Pointer to the pico i2c instance (i2c0 or i2c1).
 * @param address I2C display address.
 * @param x_flip If true, flips the display orientation horizontally.
 * @param y_flip If true, flips the display orientation vertically.
 *
 * @return true if the init went smoothly false otherwise.
 *
 */
bool ssd1306_init_i2c(ssd1306_t *p, uint8_t width, uint8_t height, i2c_inst_t *i2c_instance, uint8_t address, bool x_flip, bool y_flip);


bool ssd1306_init(ssd1306_t *p,uint8_t width, uint8_t height, uint8_t address, i2c_inst_t *instance,bool x_flip,bool y_flip);
void ssd1306_set_inversion_normal(ssd1306_t *p);
void ssd1306_set_inversion_inverted(ssd1306_t *p);
void ssd1306_set_contrast(ssd1306_t *p, uint8_t percentage);
void scroll_trial(ssd1306_t *p);
void ssd1306_stop_scroll(ssd1306_t *p);

void ssd1306_horizontal_scroll_init(ssd1306_t *p, bool right, uint8_t start_line, uint8_t end_line, uint8_t speed);
void ssd1306_scroll_start(ssd1306_t *p);
void ssd1306_scroll_stop(ssd1306_t *p);
void ssd1306_update_display(ssd1306_t *p);
void ssd1306_clear_pixel(ssd1306_t *p, int x, int y);

void ssd1306_draw_pixel(ssd1306_t *p, int x, int y);
void ssd1306_clear_display(ssd1306_t *p);
void ssd1306_fill_display(ssd1306_t *p);

bool ssd1306_dma_init(ssd1306_t *p);
void ssd1306_draw_line(ssd1306_t *p, int x1, int y1,int x2,int y2);

void ssd1306_update_display_dma(ssd1306_t *p);

void ssd1306_draw_sprite_slow(ssd1306_t *p, const sprite_t *s, int x, int y);
void ssd1306_draw_sprite_fast(ssd1306_t *p, const sprite_t *s, int x, int y);

void ssd1306_update_display_dma_test(ssd1306_t *p);

void test_dma_16bit_hardware(ssd1306_t *p);
void ssd1306_draw_rect_fast(ssd1306_t *p, int x, int y, int w, int h);
void ssd1306_draw_fill_rect_fast(ssd1306_t *p, int x, int y, int w, int h);
//*display is divided in pages, blocks 8 pixel tall,128 wide
#endif
