#include "soc_AM335x.h"
#include "beaglebone.h"
#include "interrupt.h"
#include "dmtimer.h"
#include "timers.h"


#define TIMER_CONVERT(microsec) ((0xFFFFFFFFu-(microsec*26u)))

static void DMTimer2Isr(void);
static void DMTimer3Isr(void);
static void DMTimer4Isr(void);
static void DMTimer6Isr(void);
static void DMTimer7Isr(void);

static void (*callbacks[5])();


void initTimers() 
{
	IntMasterIRQEnable();
	IntAINTCInit();
}

void configTimer(int timer, unsigned int microseconds, unsigned int autoReload, void (*callback)()) 
{
	unsigned int interrupt;
	unsigned int registers;

	switch(timer){
		case 0:
			DMTimer2ModuleClkConfig();
			interrupt = SYS_INT_TINT2;
			registers = SOC_DMTIMER_2_REGS;
			IntRegister(interrupt, DMTimer2Isr);
			break;
		case 1:
			DMTimer3ModuleClkConfig();
			interrupt = SYS_INT_TINT3;
			registers = SOC_DMTIMER_3_REGS;
			IntRegister(interrupt, DMTimer3Isr);
			break;
		case 2:
			DMTimer4ModuleClkConfig();
			interrupt = SYS_INT_TINT4;
			registers = SOC_DMTIMER_4_REGS;
			IntRegister(interrupt, DMTimer4Isr);
			break;
		case 3:
			DMTimer6ModuleClkConfig();
			interrupt = SYS_INT_TINT6;
			registers = SOC_DMTIMER_6_REGS;
			IntRegister(interrupt, DMTimer6Isr);
			break;
		case 4:
			DMTimer7ModuleClkConfig();
			interrupt = SYS_INT_TINT7;
			registers = SOC_DMTIMER_7_REGS;
			IntRegister(interrupt, DMTimer7Isr);
	}

	callbacks[timer] = callback;

    /* Set the priority */
    IntPrioritySet(interrupt, timer, AINTC_HOSTINT_ROUTE_IRQ);

    /* Enable the system interrupt */
    IntSystemEnable(interrupt);

    DMTimerPreScalerClkDisable(registers);
	DMTimerReset(registers);
    /* Load the counter with the initial count value */
    DMTimerCounterSet(registers, TIMER_CONVERT(microseconds));

    /* Load the load register with the reload count value */
    DMTimerReloadSet(registers, TIMER_CONVERT(microseconds));

    /* Configure the DMTimer for Auto-reload and compare mode */
    DMTimerModeConfigure(registers, reload);

    DMTimerIntEnable(registers, DMTIMER_INT_OVF_EN_FLAG);

    DMTimerEnable(registers);
}

void freezeTimer(int timer)
{
    switch(timer) {
        case 0: 
            DMTimerDisable(SOC_DMTIMER_2_REGS);
            break;
        case 1: 
            DMTimerDisable(SOC_DMTIMER_3_REGS);
            break;
        case 2: 
            DMTimerDisable(SOC_DMTIMER_4_REGS);
            break;
        case 3: 
            DMTimerDisable(SOC_DMTIMER_6_REGS);
            break;
        case 4: 
            DMTimerDisable(SOC_DMTIMER_7_REGS);
    }
}

unsigned long getTime(int timer) {
    // write code to return the value of the timer counter in microseconds
}

  
static void DMTimer2Isr(void)
{
    /* Disable the DMTimer interrupts */
    DMTimerIntDisable(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_IT_FLAG);

    callbacks[0]();

    /* Enable the DMTimer interrupts */
    DMTimerIntEnable(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);
}

static void DMTimer3Isr(void)
{
    /* Disable the DMTimer interrupts */
    DMTimerIntDisable(SOC_DMTIMER_3_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_3_REGS, DMTIMER_INT_OVF_IT_FLAG);

    callbacks[1]();

    /* Enable the DMTimer interrupts */
    DMTimerIntEnable(SOC_DMTIMER_3_REGS, DMTIMER_INT_OVF_EN_FLAG);
}

static void DMTimer4Isr(void)
{
    /* Disable the DMTimer interrupts */
    DMTimerIntDisable(SOC_DMTIMER_4_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_4_REGS, DMTIMER_INT_OVF_IT_FLAG);

    callbacks[2]();

    /* Enable the DMTimer interrupts */
    DMTimerIntEnable(SOC_DMTIMER_4_REGS, DMTIMER_INT_OVF_EN_FLAG);
}


static void DMTimer6Isr(void)
{
    /* Disable the DMTimer interrupts */
    DMTimerIntDisable(SOC_DMTIMER_6_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_6_REGS, DMTIMER_INT_OVF_IT_FLAG);

    callbacks[3]();

    /* Enable the DMTimer interrupts */
    DMTimerIntEnable(SOC_DMTIMER_6_REGS, DMTIMER_INT_OVF_EN_FLAG);
}

static void DMTimer7Isr(void)
{
    /* Disable the DMTimer interrupts */
    DMTimerIntDisable(SOC_DMTIMER_7_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_7_REGS, DMTIMER_INT_OVF_IT_FLAG);

    callbacks[4]();

    /* Enable the DMTimer interrupts */
    DMTimerIntEnable(SOC_DMTIMER_7_REGS, DMTIMER_INT_OVF_EN_FLAG);
}