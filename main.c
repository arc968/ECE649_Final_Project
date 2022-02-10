/*
Andrew R. Courtemanche 2020/12
*/

#define ARC_MSP_USE_GPIO
#define ARC_MSP_TYPE_msp430fr6989
#include "arc_msp_helper.h"

#include "led_panel.h"

#include "arcos.h"

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

//pin defines
#define GREEN_LED &P(9,7)
#define RED_LED &P(1,0)
#define LEFT_BTN &P(1,1)
#define RIGHT_BTN &P(1,2)

__attribute__ ((used))
__attribute__ ((noinline))
void process1(void) {
    while (true) {
        if (digitalRead(RIGHT_BTN) == LOW) {
            digitalWrite(GREEN_LED, HIGH);
        } else {
            digitalWrite(GREEN_LED, LOW);
        }
        arcos_proc_yield();
    }
}

__attribute__ ((used))
__attribute__ ((noinline))
void process2(void) {
    while (true) {
        if (digitalRead(LEFT_BTN) == LOW) {
            digitalWrite(RED_LED, HIGH);
        } else {
            digitalWrite(RED_LED, LOW);
        }
        arcos_proc_yield();
    }
}

//this is too large to fit in SRAM, so it is put in FRAM
__attribute__ ((lower))
__attribute__ ((persistent)) //by default, __attribute__ ((lower)) will place in SRAM, linking fails if this is not present
uint8_t fb[LED_PANEL_WIDTH*LED_PANEL_HEIGHT*3] = {0};

void fb_clear(void) {
    for(uint16_t i=0; i<sizeof(fb); i++) {
        fb[i] = 0;
    }
}

__attribute__((used))
__attribute__ ((noinline))
void process_render(void) {
    while (true) {
        fb_clear();
        //fill fb with gradient
        for (uint16_t x=0; x<LED_PANEL_WIDTH; x++){
            for (uint16_t y=0; y<LED_PANEL_HEIGHT; y++){
                fb[((x + (y*LED_PANEL_WIDTH))*3) + 0] += x + y;
                fb[((x + (y*LED_PANEL_WIDTH))*3) + 1] += (LED_PANEL_WIDTH - 1 - x) + (LED_PANEL_WIDTH - 1 - y);
                fb[((x + (y*LED_PANEL_WIDTH))*3) + 2] += 0;
            }
        }
        arcos_proc_yield();
        led_draw(&fb[0]);
        led_flush();
        for(volatile uint32_t delay = 100000; delay>0; delay--); //adds arbitrary delay

        fb_clear();
        //fill fb with opposite gradient
        for (uint16_t x=0; x<LED_PANEL_WIDTH; x++){
            for (uint16_t y=0; y<LED_PANEL_HEIGHT; y++){
                fb[((x + (y*LED_PANEL_WIDTH))*3) + 0] += (LED_PANEL_WIDTH - 1 - x) + (LED_PANEL_WIDTH - 1 - y);
                fb[((x + (y*LED_PANEL_WIDTH))*3) + 1] += x + y;
                fb[((x + (y*LED_PANEL_WIDTH))*3) + 2] += 0;
            }
        }
        arcos_proc_yield();
        led_draw(&fb[0]);
        led_flush();
        for(volatile uint32_t delay = 100000; delay>0; delay--); //adds arbitrary delay
    }
}

//place in upper FRAM, not SRAM to keep SRAM clear
__attribute__ ((upper))
struct arcos_proc_s process1_s;
__attribute__ ((upper))
struct arcos_proc_s process2_s;
__attribute__ ((upper))
struct arcos_proc_s process_render_s;
__attribute__ ((upper))
struct arcos_proc_s process_startup_s;

__attribute__((used))
__attribute__ ((noinline))
void process_startup(void) {
    //red LED init
    pinMode(RED_LED, MODE_OUTPUT);
    digitalWrite(RED_LED, LOW);

    //green LED init
    pinMode(GREEN_LED, MODE_OUTPUT);
    digitalWrite(GREEN_LED, LOW);

    //left BTN init
    pinMode(LEFT_BTN, MODE_INPUT_PULLUP);

    //right BTN init
    pinMode(RIGHT_BTN, MODE_INPUT_PULLUP);

    //zero out RAM, nothing should be using it. This is mostly just as a demonstration that this is running within FRAM now.
    //0x001C00-0x0023FF RAM
    for (uint16_t i=0x1C00; i<=0x0023FF; i++) {
        *((uint8_t *)i) = 0;
    }

    arcos_proc_create(&process1_s, &process1, 0, 100); //automatic stack allocation, 100 priority
    arcos_proc_start(&process1_s);
    arcos_proc_create(&process2_s, &process2, 0, 100); //automatic stack allocation, 100 priority
    arcos_proc_start(&process2_s);
    arcos_proc_create(&process_render_s, &process_render, 0x2400, 100); //place this process in SRAM (0x2400 is the top of SRAM), 100 priority
    arcos_proc_start(&process_render_s);
    //Here, this process returns and terminates. It will not run again.
}

void main(void) {
    arcos_init();
    arc_msp_setup();
    led_init();

    arcos_proc_create(&process_startup_s, &process_startup, 0, 0); //automatic stack allocation, 0 (maximum) priority
    arcos_proc_start(&process_startup_s);

    arcos_start();
}
