#include "soc_AM335x.h"
#include "beaglebone.h"
#include "interrupt.h"
#include "dmtimer.h"
#include "timers.h"


#define TIMER_CONVERT_US(microsec) 	((0xFFFFFFFFu-(microsec*26u)))

static void DMTimer2Isr(void);
static void DMTimer3Isr(void);
static void DMTimer4Isr(void);
static void DMTimer6Isr(void);
static void DMTimer7Isr(void);

static const unsigned int timerRegs[5] = {
	SOC_DMTIMER_2_REGS, 
	SOC_DMTIMER_3_REGS,
	SOC_DMTIMER_4_REGS,
	SOC_DMTIMER_6_REGS,
	SOC_DMTIMER_7_REGS
};

static const unsigned int timerInts[5] = {
	SYS_INT_TINT2,
	SYS_INT_TINT3,
	SYS_INT_TINT4,
	SYS_INT_TINT6,
	SYS_INT_TINT7
};

static void (*timerISRs[5])() = {
	DMTimer2Isr,
	DMTimer3Isr,
	DMTimer4Isr,
	DMTimer6Isr,
	DMTimer7Isr
};

static void (*timerClkConfigs[5])() = {
	DMTimer2ModuleClkConfig,
	DMTimer3ModuleClkConfig,
	DMTimer4ModuleClkConfig,
	DMTimer6ModuleClkConfig,
	DMTimer7ModuleClkConfig
};

static void (*callbacks[5])();
static unsigned int initValues[5];


void initTimers() 
{
	IntMasterIRQEnable();
	IntAINTCInit();
}

void configTimer(int timer, unsigned int microseconds, unsigned int autoReload, void (*callback)()) 
{
	timerClkConfigs[timer]();
	IntRegister(timerInts[timer],timerISRs[timer]);

	callbacks[timer] = callback;

    /* Set the priority */
    IntPrioritySet(timerInts[timer], timer, AINTC_HOSTINT_ROUTE_IRQ);

    /* Enable the system interrupt */
    IntSystemEnable(timerInts[timer]);

    DMTimerPreScalerClkDisable(timerRegs[timer]);
	DMTimerReset(timerRegs[timer]);
    /* Load the counter with the initial count value */
    DMTimerCounterSet(timerRegs[timer], initValues[timer]=TIMER_CONVERT_US(microseconds));

    /* Load the load register with the reload count value */
    DMTimerReloadSet(timerRegs[timer], TIMER_CONVERT_US(microseconds));

    /* Configure the DMTimer for Auto-reload mode or One-shot mode */
    DMTimerModeConfigure(timerRegs[timer], autoReload);

    DMTimerIntEnable(timerRegs[timer], DMTIMER_INT_OVF_EN_FLAG);

    DMTimerEnable(timerRegs[timer]);
}

void freezeTimer(int timer)
{
    DMTimerDisable(timerRegs[timer]);
}

unsigned int getElapsedTime(int timer)
{
	return (((DMTIMERCONTEXT*)timerRegs[timer])->tcrr - initValues[timer])/26;
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