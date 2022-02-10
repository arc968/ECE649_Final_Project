/*
Andrew R. Courtemanche 2020/12
*/

#ifndef LED_PANEL_GUARD
#define LED_PANEL_GUARD

#include <stdint.h>

#define LED_PANEL_WIDTH 32
#define LED_PANEL_HEIGHT 32
#define LED_CHANNEL_WIDTH 32
#define LED_CHANNEL_HEIGHT 16

//initialize required registers
void led_init(void);

//make sure LED strips have enough time to reset
void led_flush(void);

//draw given 32x32 24 bit RGB framebuffer
void led_draw(uint8_t * rgb_buf);

#endif //end include guard
