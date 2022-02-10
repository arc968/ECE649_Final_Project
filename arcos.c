/*
Andrew R. Courtemanche 2020/11
Updated 2020/12
*/

#include "arcos.h"

#include <msp430.h>
#include <string.h>

//FOR FUTURE USE
//provides a valid ISR stub for all interrupt sources
__attribute__ ((interrupt(AES256_VECTOR)))
__attribute__ ((interrupt(RTC_VECTOR)))
__attribute__ ((interrupt(LCD_C_VECTOR)))
__attribute__ ((interrupt(PORT4_VECTOR)))
__attribute__ ((interrupt(PORT3_VECTOR)))
__attribute__ ((interrupt(TIMER3_A1_VECTOR)))
__attribute__ ((interrupt(TIMER3_A0_VECTOR)))
__attribute__ ((interrupt(PORT2_VECTOR)))
__attribute__ ((interrupt(TIMER2_A1_VECTOR)))
__attribute__ ((interrupt(TIMER2_A0_VECTOR)))
__attribute__ ((interrupt(PORT1_VECTOR)))
__attribute__ ((interrupt(TIMER1_A1_VECTOR)))
__attribute__ ((interrupt(TIMER1_A0_VECTOR)))
__attribute__ ((interrupt(DMA_VECTOR)))
__attribute__ ((interrupt(USCI_B1_VECTOR)))
__attribute__ ((interrupt(USCI_A1_VECTOR)))
__attribute__ ((interrupt(TIMER0_A1_VECTOR)))
__attribute__ ((interrupt(TIMER0_A0_VECTOR)))
__attribute__ ((interrupt(ADC12_VECTOR)))
__attribute__ ((interrupt(USCI_B0_VECTOR)))
__attribute__ ((interrupt(USCI_A0_VECTOR)))
__attribute__ ((interrupt(ESCAN_IF_VECTOR)))
//__attribute__ ((interrupt(WDT_VECTOR)))
__attribute__ ((interrupt(TIMER0_B1_VECTOR)))
__attribute__ ((interrupt(TIMER0_B0_VECTOR)))
__attribute__ ((interrupt(COMP_E_VECTOR)))
__attribute__ ((interrupt(UNMI_VECTOR)))
__attribute__ ((interrupt(SYSNMI_VECTOR)))
__attribute__ ((interrupt))
static void arcos_isr_stub(void) {
    return;
}

struct arcos_kernel_s {
    uint16_t SP;
    uint8_t stack[ARCOS_CONFIG_KERNEL_SIZE_STACK];
    struct arcos_proc_s * proc_current;
    uint16_t proc_count;
    struct arcos_proc_s * proc_list[ARCOS_CONFIG_PROC_COUNT_MAX];
    uint8_t proc_stack[ARCOS_CONFIG_PROC_COUNT_MAX][ARCOS_CONFIG_PROC_STACK_SIZE_MAX];
};

//stores all information that the kernel needs
__attribute__ ((lower))
__attribute__ ((persistent))
static struct arcos_kernel_s arcos_var_kernel = {0};

//These functions are simply intended to increase code readability
//  and reduce potential mistakes. They have the inline specifier to hint that they
//  should probably not result in a true function call.
static inline void WDT_restop(void) {
    WDTCTL = WDTPW | WDTHOLD | WDTCNTCL | WDTCTL_L;
}
static inline void WDT_stop(void) {
    WDTCTL = WDTPW | WDTHOLD | WDTCTL_L;
}
static inline void WDT_reset(void) {
    WDTCTL = WDTPW | WDTCNTCL | WDTCTL_L;
}
static inline void WDT_restart(void) {
    WDTCTL = WDTPW | WDTCNTCL | (WDTCTL_L & (~WDTHOLD));
}
static inline void WDT_start(void) {
    WDTCTL = WDTPW | (WDTCTL_L & (~WDTHOLD));
}

//Triggers a reset and PUC, hard restarting the entire MCU
__attribute__ ((used))
__attribute__ ((noreturn))
__attribute__ ((naked))
static void arcos_os_pwr_reset(void) {
    WDTCTL = WDTPW | WDTSSEL__ACLK | WDTIS__64; //sets WDT to watchdog mode
    WDTCTL = 0xFF | WDTSSEL__ACLK | WDTIS__64; //intentionally writes incorrect password to WDTCTL to trigger a hard reset
    while(true); //wait
}

//simple priority round-robin scheduling
//WARNING: high priority tasks can starve lower priority tasks
__attribute__ ((noreturn))
__attribute__ ((naked))
static void arcos_os_schedule(void) {
    if (arcos_var_kernel.proc_count > 0) { //are there any processes left?
        for (uint8_t i=0; i<arcos_var_kernel.proc_count; i++) { //read through the list of processes
            if (arcos_var_kernel.proc_list[i]->status == PROC_STATE_READY) { //get the first ready process, this will be the highest priority one since list is sorted
                arcos_var_kernel.proc_current = arcos_var_kernel.proc_list[i]; //set as current process
                uint8_t ii = i+1;
                //move this process to the back of the list of processes with the same priority
                for (; (arcos_var_kernel.proc_list[ii]->priority == arcos_var_kernel.proc_current->priority) && (ii < arcos_var_kernel.proc_count); ii++) {
                    arcos_var_kernel.proc_list[ii-1] = arcos_var_kernel.proc_list[ii];
                }
                arcos_var_kernel.proc_list[ii-1] = arcos_var_kernel.proc_current;
                break;
            }
        }
    } else {
        arcos_os_pwr_reset(); //There are no more processes left to schedule. This is assumed to be a mistake, so restart the MCU
    }

    __asm(" BIC.B %0, %1\n"::"r"(WDTIFG), "r"(SFRIFG1_L)); //clear WDTIFG using BIC.B, as recommended in User's Guide page 74
    WDT_restart(); //restart the WDT
    //_enable_interrupts(); //not needed because RETI will restore SR and enable interrupts
    arcos_var_kernel.proc_current->status = PROC_STATE_RUNNING; //mark the selected process as running
    __set_SP_register(arcos_var_kernel.proc_current->SP); //change the stack pointer to the process
    __asm(" POPM.A #12,R15\n"); //restore context
    __asm(" RETI\n"); //return control flow to process
}

//kernel entry point after startup, process pre-emption, etc
__attribute__ ((noreturn))
__attribute__ ((naked))
static void arcos_os_run(void) {
    WDT_stop(); //stop the WDT
    //other features added here
    arcos_os_schedule(); //schedule the next process
}

//ISR called after timeslice expires
//saves context of current process and returns control flow to kernel
__attribute__ ((interrupt(WDT_VECTOR)))
__attribute__ ((naked))
static void arcos_os_isr_timeout_slice(void) {
    //PC and SR are already pushed by the interrupt
    //Pushes R4-R15 to the stack, saving all 20 bits. Each register uses 4 bytes, so total size is 48bytes
    __asm(" PUSHM.A #12,R15\n");
    //At this point, process context is saved except SP
    uint16_t proc_SP = __get_SP_register(); //get SP
    arcos_var_kernel.proc_current->SP = proc_SP; //save process SP
    arcos_var_kernel.proc_current->status = PROC_STATE_READY; //mark process as ready to run

    __set_SP_register(arcos_var_kernel.SP); //change stack pointer to kernel
    __asm(" PUSHX.A %0\n"::"r"(&arcos_os_run)); //push address of arcos_os_run() to stack
    __asm(" RETA\n"); //pop address and return control flow to kernel
}

//sorts processes by priority
//very crappy sorting algorithm
static inline void arcos_os_proc_sort(void) {
    for (uint8_t i=0; i<arcos_var_kernel.proc_count; i++) {
        struct arcos_proc_s * temp = arcos_var_kernel.proc_list[i];
        int16_t ii = i - 1;
        for (; ii >= 0 && arcos_var_kernel.proc_list[ii]->priority > temp->priority; ii--) {
            arcos_var_kernel.proc_list[ii+1] = arcos_var_kernel.proc_list[ii];
        }
        arcos_var_kernel.proc_list[ii+1] = temp;
    }
}

//removes process from process list
void arcos_proc_terminate(struct arcos_proc_s * handle) {
    uint16_t GIE_BACKUP = _get_SR_register() & GIE; //store GIE
    __asm(" DINT \n NOP \n"); //disable interrupts

    handle->status = PROC_STATE_TERMINATED;

    //finds process index from pointer
    for (uint8_t i=0; i<arcos_var_kernel.proc_count; i++) {
        if (arcos_var_kernel.proc_list[i] == handle) {
            arcos_var_kernel.proc_list[i] = arcos_var_kernel.proc_list[arcos_var_kernel.proc_count-1];
        }
    }
    arcos_var_kernel.proc_count--;
    arcos_os_proc_sort();

    __asm(" BIS.B %0, SR \n NOP \n"::"r"(GIE_BACKUP)); //restore GIE
}

//runs when a process returns
__attribute__ ((noreturn))
__attribute__ ((naked))
static void arcos_os_proc_return(void) {
    _disable_interrupts();

    arcos_proc_terminate(arcos_var_kernel.proc_current);

    __set_SP_register(arcos_var_kernel.SP);
    __asm(" PUSHX.A %0\n"::"r"(&arcos_os_run));
    __asm(" RETA\n");
}

//initial kernel entry point
__attribute__ ((noreturn))
__attribute__ ((naked))
void arcos_start(void) {
    _disable_interrupts();

    arcos_var_kernel.SP = (uint16_t) arcos_var_kernel.stack + sizeof(arcos_var_kernel.stack); //valid cast because we know the stacks are in the lower 64K of memory

    __set_SP_register(arcos_var_kernel.SP);
    __asm(" PUSHX.A %0\n"::"r"(&arcos_os_run));
    __asm(" RETA\n");
}

//initial configuration of the core, sets up watchdog, clocks, timers, etc.
//should be called immediately after boot
void arcos_init(void) {
    _disable_interrupts();
    WDTCTL = WDTPW | WDTHOLD | WDTSSEL__ACLK | WDTTMSEL | WDTCNTCL | WDTIS__64; // Configure WDT
    __asm(" BIC.B %0, %1\n"::"r"(WDTIFG), "r"(SFRIFG1_L)); //clear WDTIFG using BIC.B, as recommended in User's Guide page 74
    SFRIE1 = WDTIE; //enable WDT interrupt

    FRCTL0 = FRCTLPW; //unlock FRCTL registers
    FRCTL0_L = NWAITS_1; //set FRAM wait mode to 1 cycle (max FRAM access frequency is 8Mhz, we are setting CPU to 16Mhz)
    FRCTL0_H = 0; //lock FRCTL registers

    CSCTL0 = CSKEY; //unlock CSCTLx registers
    CSCTL1 = DCORSEL | DCOFSEL_4; //set DCO to 16Mhz
    CSCTL2 = SELA__LFXTCLK | SELS__MODCLK | SELM__DCOCLK; //select clock sources
    CSCTL3 = DIVA__1 | DIVS__2 | DIVM__1; //AUX 32768Hz, SM 2.5Mhz, M 16Mhz
    //CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK; //select clock sources
    //CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1; //AUX 32768Hz, SM 16Mhz, M 16Mhz
    //CSCTL3 = (CSCTL3 & ~((DIVS0 | DIVS1 | DIVS2) | (DIVM0 | DIVM1 | DIVM2))) | (DIVS_0 | DIVM_0); //set SMCLK and MCLK dividers to /1
    CSCTL0_H = 0; //lock CSCTLx registers

    //initialize unused ports to make compiler shut up
    PADIR = 0x00;
    PAOUT = 0x00;
    PBDIR = 0x00;
    PBOUT = 0x00;
    PCDIR = 0x00;
    PCOUT = 0x00;
    PDDIR = 0x00;
    PDOUT = 0x00;
    PEDIR = 0x00;
    PEOUT = 0x00;
}

//yields timeslice to another process by immediately setting WDT interrupt flag
inline void arcos_proc_yield(void) {
    //SFRIFG1 = SFRIFG1 | WDTIFG;
    __asm(" BIS.B %0, %1 \n NOP \n"::"i"(WDTIFG), "r"(SFRIFG1));
}

//marks process as ready to run
void arcos_proc_start(struct arcos_proc_s * handle) {
    uint16_t GIE_BACKUP = _get_SR_register() & GIE; //store GIE
    __asm(" DINT \n NOP \n"); //disable interrupts

    handle->status = PROC_STATE_READY; //TODO: add error checking

    __asm(" BIS.B %0, SR \n NOP \n"::"r"(GIE_BACKUP)); //restore GIE
}

/*
 * KNOWN ISSUE: Stack selection is based on number of processes, if a process ever terminates and a new one is created, stack pointers will then overlap
 */
//0 is highest priority, 255 is lowest priority
//initializes a arcos_proc_s struct and internal ARCOS variables
void arcos_proc_create(struct arcos_proc_s * handle, void (*callback)(void), uint16_t SP, uint8_t priority) {
    uint16_t GIE_BACKUP = _get_SR_register() & GIE; //store GIE
    __asm(" DINT \n NOP \n"); //disable interrupts

    //initialize arcos_proc_s struct
    handle->priority = priority;
    handle->status = PROC_STATE_STOPPED;
    if (SP) {
        handle->SP = SP; //use specific stack pointer, if provided
    } else {
        handle->SP = (uint16_t) arcos_var_kernel.proc_stack[arcos_var_kernel.proc_count] + ARCOS_CONFIG_PROC_STACK_SIZE_MAX; //assign a process stack
    }
    handle->callback = callback;

    //manually manipulate process stack
    handle->SP -= 4;
    *((uintptr_t *)handle->SP) = (uintptr_t) &arcos_os_proc_return; //push return address of arcos_os_proc_return()
    handle->SP -= 2;
    *((uint16_t *)handle->SP) = ((uint32_t)callback) & 0x0000FFFF; //push process PC
    handle->SP -= 2;
    *((uint16_t *)handle->SP) = ((((uint32_t)callback) & 0x000F0000) >> 4) | (0b00001000 & 0x1FF); //push process SR
    handle->SP -= 4 * 12; //push 12 empty registers

    arcos_var_kernel.proc_list[arcos_var_kernel.proc_count] = handle; //add pointer to process list
    arcos_var_kernel.proc_count++;
    arcos_os_proc_sort();

    __asm(" BIS.B %0, SR \n NOP \n"::"r"(GIE_BACKUP)); //restore GIE
}
