/*
Andrew R. Courtemanche 2020/11
Updated 2020/12
*/

/*
RTC_C
32,768Hz LFXT crystal
2,048 bytes RAM
48,000 bytes 16-bit address FRAM
81,920 bytes 20-bit address FRAM
defaults:
    MCLK DCO 1MHz
    SMCLK DCO 1MHz
    ACLK REFO(?) 32,768 Hz
DCO:
    (24MHz / 21MHz / 16MHz / 8MHz / 7MHz / 5.33MHz / 4MHz / 3.5MHz / 2.67 MHz / 1MHz)
    1MHz
    2.67MHz
    3.5MHz
    4MHz
    5.33MHz
    7MHz
    8MHz
    16MHz
    21MHz
    24MHz
source dividers for MCLK, SMCLK, ACLK:
    1
    2
    4
    8
    16
    32
MCLK sources:
    LFXTCLK  (32768Hz / 16384Hz / 8192Hz / 4096Hz / 2048Hz / 1024Hz)
    VLOCLK   (10kHz / 5kHz / 2.5kHz / 1.25kHz / ~625Hz / 312.5Hz)
    LFMODCLK (~39kHz / ~19.5kHz / ~9.77kHz / ~4.88kHz / ~2.44kHz / ~1.22kHz)
    DCOCLK   (configurable)
    MODCLK   (5MHz / 2.5MHz / 1.25MHz / ~625kHz / ~312.6kHz / ~156.25kHz)
    HFXTCLK  (Not available)
SMCLK sources:
    LFXTCLK  (32768Hz / 16384Hz / 8192Hz / 4096Hz / 2048Hz / 1024Hz)
    VLOCLK   (10kHz / 5kHz / 2.5kHz / 1.25kHz / ~625Hz / 312.5Hz)
    LFMODCLK (~39kHz / ~19.5kHz / ~9.77kHz / ~4.88kHz / ~2.44kHz / ~1.22kHz)
    DCOCLK   (configurable)
    MODCLK   (5MHz / 2.5MHz / 1.25MHz / ~625kHz / ~312.6kHz / ~156.25kHz)
    HFXTCLK  (Not available)
ACLK sources:
    LFXTCLK  (32768Hz / 16384Hz / 8192Hz / 4096Hz / 2048Hz / 1024Hz)
    VLOCLK   (10kHz / 5kHz / 2.5kHz / 1.25kHz / ~625Hz / ~312.5Hz)
    LFMODCLK (~39kHz / ~19.5kHz / ~9.77kHz / ~4.88kHz / ~2.44kHz / ~1.22kHz)

WDT sources:
    SMCLK
    ACLK
    VLOCLK (10kHz)
WDT intervals:
    CLK/2^31 (18:12:16 at 32.768kHz)
    CLK/2^27 (01:08:16 at 32.768kHz)
    CLK/2^23 (00:04:16 at 32.768kHz)
    CLK/2^19 (00:00:16 at 32.768kHz)
    CLK/2^15 (1 s at 32.768kHz)
    CLK/2^13 (250ms at 32.768kHz)
    CLK/2^9  (15.625ms at 32.768kHz
    CLK/2^6  (1.95ms at 32.768kHz)

*/

/*
2,048 bytes RAM (fastest):
0x001C00-0x0023FF - general space
    0x001C00-0x00.... - global variables
    0x00....-0x00.... - 1024 bytes process contexts (32 contexts of 16 2-byte registers)
    0x00....-0x0023FA - OS stack
    0x0023FC-0x0023FD - main() return pointer
    0x0023FE-0x0023FF - NULL

48,000 bytes 16-bit address FRAM:
0x004400-0x00FF7F - general space
    0x004400-0x007F7F 15,232 bytes - code / data
    0x007F80-0x00FF7F 32,768 bytes - arcos process stacks (32x1024)
0x00FF80-0x00FFFF - interrupt vectors and signatures

81,920 bytes 20-bit address FRAM (slowest):
0x010000-0x023FFFF - general space
    0x010000-0x0..... - code
    0x0.....-0x023FFF - heap

 */

#ifndef ARCOS_GUARD
#define ARCOS_GUARD

#include <stdint.h>
#include <stdbool.h>

#include "arcos_config.h"

//possible states of a process
enum arcos_proc_status_e {
    PROC_STATE_UNINITIALIZED = 0,
    PROC_STATE_TERMINATED,
    PROC_STATE_STOPPED,
    //PROC_STATE_SUSPENDED, //future use
    //PROC_STATE_BLOCKED, //future use
    PROC_STATE_READY,
    PROC_STATE_RUNNING,
};

//describes a particular process
//included in header so size is known
struct arcos_proc_s {
    uint8_t priority;
    enum arcos_proc_status_e status;
    uint16_t SP;
    void (*callback)(void);
};

//marks process as ready for execution
void arcos_proc_start(struct arcos_proc_s * handle);

//terminates specified process
void arcos_proc_terminate(struct arcos_proc_s * handle);

//yields timeslice to another process
void arcos_proc_yield(void);

//initializes a arcos_proc_s struct and internal ARCOS variables
//0 is highest priority, 255 is lowest priority
void arcos_proc_create(struct arcos_proc_s * handle, void (*callback)(void), uint16_t SP, uint8_t priority);

//initial configuration of the core, sets up watchdog, clocks, timers, etc.
//should be called immediately after boot
void arcos_init(void);

//hands execution over to ARCOS
//will never return, and current stack is invalidated
void arcos_start(void);

#endif //end ARCOS_GUARD
