/*
Andrew R. Courtemanche 2020/11
Updated 2020/12
*/

#ifndef ARC_MSP_HELPER_GUARD
#define ARC_MSP_HELPER_GUARD

#include <stdint.h>
#include <stdbool.h>

//For future use to support different MSP430 variants
#ifdef ARC_MSP_TYPE_msp430fr6989
    #ifdef ARC_MSP_TYPE
        #error Multiple MSP types defined
    #endif //end ARC_MSP_TYPE
    #define ARC_MSP_TYPE
#endif //end ARC_MSP_TYPE_msp430fr6989
#ifndef ARC_MSP_TYPE
    #error No MSP type defined
#endif //end ARC_MSP_TYPE

#ifdef ARC_MSP_USE_ALL
    #define ARC_MSP_USE_GPIO
#endif //end ARC_MSP_USE_ALL

//Convenience defines
#define NULL 0

#define LOW 0
#define HIGH 1

#define DISABLE 0
#define ENABLE 1

#define RISING_EDGE 0
#define FALLING_EDGE 1
//end convenience defines

#ifdef ARC_MSP_USE_GPIO

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

    //references a specific pin on a specific port
    struct portPin_s {
        const struct port_s * port;
        const uint8_t pin;
    };

    //stores the bit states for each register for a given GPIO configuration
    struct pin_mode_s {
        uint8_t dir;
        uint8_t out;
        uint8_t ren;
    };
    extern const struct pin_mode_s pin_mode_input;
    extern const struct pin_mode_s pin_mode_input_pullup;
    extern const struct pin_mode_s pin_mode_input_pulldown;
    extern const struct pin_mode_s pin_mode_output;

    //used so uninitialized ISRs return correctly
    void dummy(void);

    //stores the ISR callback for each pin on interrupt-enabled ports
    //not included in port_s struct so that ports that do not support interrupts do not waste memory
    struct port_callbacks_s {
        void (*callbacks[8])(void);
    };
    extern struct port_callbacks_s port1_cb;
    extern struct port_callbacks_s port2_cb;
    extern struct port_callbacks_s port3_cb;
    extern struct port_callbacks_s port4_cb;

    //stores memory-mapped register addresses for a given port
    struct port_s {
        volatile uint8_t * dir_addr;
        volatile uint8_t * out_addr;
        volatile uint8_t * ren_addr;
        volatile uint8_t * in_addr;
        volatile uint8_t * ie_addr;
        volatile uint8_t * ies_addr;
        volatile uint8_t * ifg_addr;
        volatile uint8_t * sel0_addr;
        volatile uint8_t * sel1_addr;
        volatile uint8_t * selc_addr;
        struct port_callbacks_s * callbacks;
    };
    extern const struct port_s port1_v;
    extern const struct port_s port2_v;
    extern const struct port_s port3_v;
    extern const struct port_s port4_v;
    extern const struct port_s port5_v;
    extern const struct port_s port6_v;
    extern const struct port_s port7_v;
    extern const struct port_s port8_v;
    extern const struct port_s port9_v;
    extern const struct port_s port10_v;
    extern const struct port_s * port1;
    extern const struct port_s * port2;
    extern const struct port_s * port3;
    extern const struct port_s * port4;
    extern const struct port_s * port5;
    extern const struct port_s * port6;
    extern const struct port_s * port7;
    extern const struct port_s * port8;
    extern const struct port_s * port9;
    extern const struct port_s * port10;

    //bit and bitmask LUTs as a performance optimization, shifting is relatively slow on the MSP430
    extern const uint8_t pinMasks[8];
    extern const uint8_t pinBits[8];

    //Convenience defines
    #define MODE_INPUT (&pin_mode_input)
    #define MODE_INPUT_PULLUP (&pin_mode_input_pullup)
    #define MODE_INPUT_PULLDOWN (&pin_mode_input_pulldown)
    #define MODE_OUTPUT (&pin_mode_output)

    #define P(X,Y) ((struct portPin_s){port##X, Y})
    //end convenience defines

#endif //end ARC_MSP_USE_GPIO

#ifdef ARC_MSP_USE_GPIO
    //configures an interrupt for a specific pin and optionally enables it
    void pinInterrupt(const struct portPin_s * portPin, uint8_t interruptEnable, uint8_t edge, void (*isr_callback)(void));
    //convenience function to configure interrupts on a group of pins
    void group_pinInterrupt(const struct portPin_s (*portPins)[], const uint16_t count, uint8_t interruptEnable, uint8_t edge, void (*isr_callback)(void));
    //convenience function to enable interrupt on a single pin
    void pinInterruptEnable(const struct portPin_s * portPin);
    //convenience function to enable interrupts on a group of pins
    void group_pinInterruptEnable(const struct portPin_s (*portPins)[], const uint16_t count);
    //convenience function to disable interrupt on a single pin
    void pinInterruptDisable(const struct portPin_s * portPin);
    //convenience function to disable interrupts on a group of pins
    void group_pinInterruptDisable(const struct portPin_s (*portPins)[], const uint16_t count);

    //configures a pin for GPIO using one of: MODE_INPUT, MODE_INPUT_PULLUP, MODE_INPUT_PULLDOWN, MODE_OUTPUT
    void pinMode(const struct portPin_s * portPin, const struct pin_mode_s * mode);
    //convenience function to set a list of pins to the same GPIO mode
    void group_pinMode(const struct portPin_s (*portPins)[], const uint16_t count, const struct pin_mode_s * mode);

    //selects a function for a GPIO pin
    void pinFunc(const struct portPin_s * portPin, const uint8_t func);
    //convenience function to set a list of pins to the same GPIO function
    void group_pinFunc(const struct portPin_s (*portPins)[], const uint16_t count, const uint8_t func);

    //sets a given pin to either HIGH or LOW
    //WARNING: does NOT check if pin is set to output
    void digitalWrite(const struct portPin_s * portPin, uint8_t val);
    //convenience function to set a list of pins to either HIGH or LOW
    void group_digitalWrite(const struct portPin_s (*portPins)[], const uint16_t count, uint8_t val);

    //reads the current state of a pin
    //WARNING: does NOT check if pin is set to input
    uint8_t digitalRead(const struct portPin_s * portPin);
    //reads the current state of a group of pins
    void group_digitalRead(const struct portPin_s (*portPins)[], const uint16_t count, uint8_t (*result)[]);
#endif //end ARC_MSP_USE_GPIO

void arc_msp_setup(void);

#endif //end include guard
