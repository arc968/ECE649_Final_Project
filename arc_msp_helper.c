/*
Andrew R. Courtemanche 2020/11
Updated 2020/12
*/

#define ARC_MSP_USE_ALL
#define ARC_MSP_TYPE_msp430fr6989
#include "arc_msp_helper.h"

#include <msp430.h>

//PxDIR is 0 for input
//PxDIR is 1 for output
//PxOUT is 0 for pull-down
//PxOUT is 1 for pull-up
//PxREN is 0 for no pull resistor
//PxREN is 1 for pull resistor
//PxIE is 0 for interrupt disable
//PxIE is 1 for interrupt enable
//PxIES is 0 for rising edge
//PxIES is 1 for falling edge
//PxIFG is 0 when no interrupt is pending
//PxIFG is 1 when interrupt is pending
//WARNING Writing to PxIES may set corresponding interrupt flag!!!

const struct pin_mode_s pin_mode_input =          {.dir = 0, .out = 0, .ren = 0};
const struct pin_mode_s pin_mode_input_pullup =   {.dir = 0, .out = 1, .ren = 1};
const struct pin_mode_s pin_mode_input_pulldown = {.dir = 0, .out = 0, .ren = 1};
const struct pin_mode_s pin_mode_output =         {.dir = 1, .out = 0, .ren = 0};

struct port_callbacks_s port1_cb = {{&dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy}};
struct port_callbacks_s port2_cb = {{&dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy}};
struct port_callbacks_s port3_cb = {{&dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy}};
struct port_callbacks_s port4_cb = {{&dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy}};

const struct port_s port1_v  = {&P1DIR, &P1OUT, &P1REN, &P1IN, &P1IE, &P1IES, &P1IFG, &P1SEL0, &P1SEL1, &P1SELC, &port1_cb}, * port1 =  &port1_v;
const struct port_s port2_v  = {&P2DIR, &P2OUT, &P2REN, &P2IN, &P2IE, &P2IES, &P2IFG, &P2SEL0, &P2SEL1, &P2SELC, &port2_cb}, * port2 =  &port2_v;
const struct port_s port3_v  = {&P3DIR, &P3OUT, &P3REN, &P3IN, &P3IE, &P3IES, &P3IFG, &P3SEL0, &P3SEL1, &P3SELC, &port3_cb}, * port3 =  &port3_v;
const struct port_s port4_v  = {&P4DIR, &P4OUT, &P4REN, &P4IN, &P4IE, &P4IES, &P4IFG, &P4SEL0, &P4SEL1, &P4SELC, &port4_cb}, * port4 =  &port4_v;
const struct port_s port5_v  = {&P5DIR, &P5OUT, &P5REN, &P5IN, NULL, NULL, NULL, &P5SEL0, &P5SEL1, &P5SELC, NULL},   * port5 =  &port5_v;
const struct port_s port6_v  = {&P6DIR, &P6OUT, &P6REN, &P6IN, NULL, NULL, NULL, &P6SEL0, &P6SEL1, &P6SELC, NULL},   * port6 =  &port6_v;
const struct port_s port7_v  = {&P7DIR, &P7OUT, &P7REN, &P7IN, NULL, NULL, NULL, &P7SEL0, &P7SEL1, &P7SELC, NULL},   * port7 =  &port7_v;
const struct port_s port8_v  = {&P8DIR, &P8OUT, &P8REN, &P8IN, NULL, NULL, NULL, &P8SEL0, &P8SEL1, &P8SELC, NULL},   * port8 =  &port8_v;
const struct port_s port9_v  = {&P9DIR, &P9OUT, &P9REN, &P9IN, NULL, NULL, NULL, &P9SEL0, &P9SEL1, &P9SELC, NULL},   * port9 =  &port9_v;
const struct port_s port10_v = {&P10DIR, &P10OUT, &P10REN, &P10IN, NULL, NULL, NULL, &P10SEL0, &P10SEL1, &P10SELC, NULL},  * port10 = &port10_v;

//bit and bitmask LUTs as a performance optimization, shifting is relatively slow on the MSP430
const uint8_t pinMasks[8] = {(uint8_t)~(0x1<<0), (uint8_t)~(0x1<<1), (uint8_t)~(0x1<<2), (uint8_t)~(0x1<<3), (uint8_t)~(0x1<<4), (uint8_t)~(0x1<<5), (uint8_t)~(0x1<<6), (uint8_t)~(0x1<<7)};
const uint8_t pinBits[8] =  { (0x1<<0),  (0x1<<1),  (0x1<<2),  (0x1<<3),  (0x1<<4),  (0x1<<5),  (0x1<<6),  (0x1<<7)};

//Looks up and calls the specific callback for a given pin.
//This is really too much work for an ISR, but it's mighty convenient
static void ISR_HANDLER(const struct port_s * port ) {
    uint8_t ifg = *(port->ifg_addr); //get interrupt flags for this port
    uint8_t i=0;
    for (; i<8; i++) { //check all 8 flags for this port
        if (ifg & 0x1) { //check if flag is set
            (port->callbacks->callbacks[i])(); //call callback
        }
        ifg = ifg >> 1; //get next flag
    }
    *(port->ifg_addr) = 0; //clear interrupt flag
}

//Interrupt Service Routines for each port
__attribute__ ((interrupt(PORT1_VECTOR)))
static void Port_1(void) {
    ISR_HANDLER(port1);
}
__attribute__ ((interrupt(PORT2_VECTOR)))
static void Port_2(void) {
    ISR_HANDLER(port2);
}
__attribute__ ((interrupt(PORT3_VECTOR)))
static void Port_3(void) {
    ISR_HANDLER(port3);
}
__attribute__ ((interrupt(PORT4_VECTOR)))
static void Port_4(void) {
    ISR_HANDLER(port4);
}

void dummy(void){
    return;
}

//configures an interrupt for a specific pin and optionally enables it
void pinInterrupt(const struct portPin_s * portPin, uint8_t interruptEnable, uint8_t edge, void (*isr_callback)(void)) {
    if (portPin->port->callbacks == NULL) return; //early return if port is not interrupt capable
    //_disable_interrupt();
    *(portPin->port->ie_addr) = (*(portPin->port->ie_addr) & pinMasks[portPin->pin]) | (pinBits[portPin->pin] * interruptEnable); //optionally enable interrupt
    *(portPin->port->ies_addr) = (*(portPin->port->ies_addr) & pinMasks[portPin->pin]) | (pinBits[portPin->pin] * edge); //configure trigger edge
    *(portPin->port->ifg_addr) = (*(portPin->port->ifg_addr) & pinMasks[portPin->pin]); //clear ifg, just in case
    portPin->port->callbacks->callbacks[portPin->pin] = isr_callback; //store callback
    //_enable_interrupt();
}
//convenience function to configure interrupts on a group of pins
void group_pinInterrupt(const struct portPin_s (*portPins)[], const uint16_t count, uint8_t interruptEnable, uint8_t edge, void (*isr_callback)(void)) {
    uint8_t i=0;
    for (; i<count; i++) {
        pinInterrupt(&(*portPins)[i], interruptEnable, edge, isr_callback);
    }
}
//convenience function to enable interrupt on a single pin
void pinInterruptEnable(const struct portPin_s * portPin) {
    if (portPin->port->callbacks == NULL) return; //early return if port is not interrupt capable
    //_disable_interrupt();
    *(portPin->port->ie_addr) = (*(portPin->port->ie_addr) & pinMasks[portPin->pin]) | (pinBits[portPin->pin]); //enable interrupt
    *(portPin->port->ifg_addr) = (*(portPin->port->ifg_addr) & pinMasks[portPin->pin]); //clear ifg
    //_enable_interrupt();
}
//convenience function to enable interrupts on a group of pins
void group_pinInterruptEnable(const struct portPin_s (*portPins)[], const uint16_t count) {
    uint8_t i=0;
    for (; i<count; i++) {
        pinInterruptEnable(&(*portPins)[i]);
    }
}
//convenience function to disable interrupt on a single pin
void pinInterruptDisable(const struct portPin_s * portPin) {
    if (portPin->port->callbacks == NULL) return; //early return if port is not interrupt capable
    //_disable_interrupt();
    *(portPin->port->ie_addr) = (*(portPin->port->ie_addr) & pinMasks[portPin->pin]); //disable interrupt
    *(portPin->port->ifg_addr) = (*(portPin->port->ifg_addr) & pinMasks[portPin->pin]); //clear ifg
    //_enable_interrupt();
}
//convenience function to disable interrupts on a group of pins
void group_pinInterruptDisable(const struct portPin_s (*portPins)[], const uint16_t count) {
    uint8_t i=0;
    for (; i<count; i++) {
        pinInterruptDisable(&(*portPins)[i]);
    }
}

//configures a pin for GPIO using one of: MODE_INPUT, MODE_INPUT_PULLUP, MODE_INPUT_PULLDOWN, MODE_OUTPUT
void pinMode(const struct portPin_s * portPin, const struct pin_mode_s * mode) {
    *(portPin->port->dir_addr) = ((*(portPin->port->dir_addr)) & pinMasks[portPin->pin]) | (pinBits[portPin->pin] * mode->dir);
    *(portPin->port->out_addr) = ((*(portPin->port->out_addr)) & pinMasks[portPin->pin]) | (pinBits[portPin->pin] * mode->out);
    *(portPin->port->ren_addr) = ((*(portPin->port->ren_addr)) & pinMasks[portPin->pin]) | (pinBits[portPin->pin] * mode->ren);
}
//convenience function to set a list of pins to the same GPIO mode
void group_pinMode(const struct portPin_s (*portPins)[], const uint16_t count, const struct pin_mode_s * mode) {
    uint8_t i=0;
    for (; i<count; i++) {
        pinMode(&(*portPins)[i], mode);
    }
}

//selects a function for a GPIO pin
void pinFunc(const struct portPin_s * portPin, const uint8_t func) {
    *(portPin->port->sel0_addr) = ((*(portPin->port->sel0_addr)) & pinMasks[portPin->pin]) | (pinBits[portPin->pin] * (func & 0b01));
    *(portPin->port->sel1_addr) = ((*(portPin->port->sel1_addr)) & pinMasks[portPin->pin]) | (pinBits[portPin->pin] * ((func & 0b10)>>1));
    //*(portPin->port->selc_addr) = ((*(portPin->port->selc_addr)) & pinMasks[portPin->pin]) | (pinBits[portPin->pin] * (func & 0b11)); //dunno
}
//convenience function to set a list of pins to the same GPIO function
void group_pinFunc(const struct portPin_s (*portPins)[], const uint16_t count, const uint8_t func) {
    uint8_t i=0;
    for (; i<count; i++) {
        pinFunc(&(*portPins)[i], func);
    }
}

//sets a given pin to either HIGH or LOW
//WARNING: does NOT check if pin is set to output
void digitalWrite(const struct portPin_s * portPin, uint8_t val) {
    *(portPin->port->out_addr) = ((*(portPin->port->out_addr)) & pinMasks[portPin->pin]) | (pinBits[portPin->pin] * val);
}
//convenience function to set a list of pins to either HIGH or LOW
void group_digitalWrite(const struct portPin_s (*portPins)[], const uint16_t count, uint8_t val) {
    uint8_t i=0;
    for (; i<count; i++) {
        digitalWrite(&(*portPins)[i], val);
    }
}

//reads the current state of a pin
//WARNING: does NOT check if pin is set to input
uint8_t digitalRead(const struct portPin_s * portPin) {
    return ((*(portPin->port->in_addr)) & pinBits[portPin->pin]) >> portPin->pin;
}
//reads the current state of a group of pins
void group_digitalRead(const struct portPin_s (*portPins)[], const uint16_t count, uint8_t (*result)[]) {
    uint8_t i=0;
    for (; i<count; i++) {
        (*result)[i] = digitalRead(&(*portPins)[i]);
    }
}

void arc_msp_setup(void) {
    PM5CTL0 &= ~LOCKLPM5;
}
