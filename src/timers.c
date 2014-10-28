#include "soc_AM335x.h"
#include "beaglebone.h"
#include "interrupt.h"
#include "dmtimer.h"
#include "timers.h"
#include "cpu.h"
#include "pin_mux.h"


#define TIMER_CONVERT_US(microsec)  (0xFFFFFFFFu-((microsec)*24u))

#define DMTimerWaitForWrite(reg, baseAdd)   \
            if(HWREG(baseAdd + DMTIMER_TSICR) & DMTIMER_TSICR_POSTED)\
            while((reg & DMTimerWritePostedStatusGet(baseAdd)));

#define REG_IDX_SHIFT                  (0x05)
#define REG_BIT_MASK                   (0x1F)


static void DMTimer2Isr(void);
static void DMTimer3Isr(void);
static void DMTimer4Isr(void);
static void DMTimer6Isr(void);
static void DMTimer7Isr(void);

// Used to index the timers' registers more easily
static const unsigned int timerRegs[5] = {
	SOC_DMTIMER_2_REGS, 
	SOC_DMTIMER_3_REGS,
	SOC_DMTIMER_4_REGS,
	SOC_DMTIMER_6_REGS,
	SOC_DMTIMER_7_REGS
};

// Used to index the timers' interrupts more easily
static const unsigned int timerInts[5] = {
	SYS_INT_TINT2,
	SYS_INT_TINT3,
	SYS_INT_TINT4,
	SYS_INT_TINT6,
	SYS_INT_TINT7
};

// Used to index the timers' ISRs more easily
static void (*timerISRs[5])() = {
	DMTimer2Isr,
	DMTimer3Isr,
	DMTimer4Isr,
	DMTimer6Isr,
	DMTimer7Isr
};

// Used to index the timers' clock config functions more easily
static void (*timerClkConfigs[5])() = {
	DMTimer2ModuleClkConfig,
	DMTimer3ModuleClkConfig,
	DMTimer4ModuleClkConfig,
	DMTimer6ModuleClkConfig,
	DMTimer7ModuleClkConfig
};

// Keeps track of the functions that should be called when the timer expires
static void (*callbacks[5])();

// Keeps track of the initial value so that we can calculate the elapsed
static unsigned int initValues[5];


void initTimers() 
{
    /* Enable CPU interrupts */
	CPUirqe();
    /* Initialize the interrupt controller */
	IntAINTCInit();
}

void runTimer(int timer, unsigned int interval) {

    /* Stop the timer if it's already running */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) &= ~DMTIMER_TCLR_ST;

    /* Set the counter value */
    HWREG(timerRegs[timer] + DMTIMER_TCRR) = initValues[timer]=TIMER_CONVERT_US(interval);

    /* Load the register with the re-load value */
    HWREG(timerRegs[timer] + DMTIMER_TLDR) = TIMER_CONVERT_US(interval);

    /* Start the timer */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) |= DMTIMER_TCLR_ST;

}

void configTimer(int timer, unsigned int autoReload, void (*callback)()) 
{
    /* Configure the timer clocks */
	timerClkConfigs[timer]();

    /* Register the ISR with the interrupt */
	IntRegister(timerInts[timer],timerISRs[timer]);

    /* Set the desired callback function */
	callbacks[timer] = callback;

    /* Set the priority */
    HWREG(SOC_AINTC_REGS + INTC_ILR(timerInts[timer])) =
                                 ((timer << INTC_ILR_PRIORITY_SHIFT)
                                   & INTC_ILR_PRIORITY)
                                 | AINTC_HOSTINT_ROUTE_IRQ ;

    /* Enable the system interrupt */
    __asm(" dsb");
    
    /* Disable the system interrupt in the corresponding MIR_CLEAR register */
    HWREG(SOC_AINTC_REGS + INTC_MIR_CLEAR(timerInts[timer] >> REG_IDX_SHIFT))
                                   = (0x01 << (timerInts[timer] & REG_BIT_MASK));

    /* Disable Pre-scaler clock */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) &= ~DMTIMER_TCLR_PRE;

    /* Clear the POSTED field of TSICR */
    HWREG(timerRegs[timer] + DMTIMER_TSICR) &= ~DMTIMER_TSICR_POSTED;

    /* Write to the POSTED field of TSICR */
    HWREG(timerRegs[timer] + DMTIMER_TSICR) |= (DMTIMER_NONPOSTED & DMTIMER_TSICR_POSTED);

    /* Clear the AR and CE field of TCLR */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) &= ~(DMTIMER_TCLR_AR | DMTIMER_TCLR_CE);

    /* Set the timer mode in TCLR register */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) |= (autoReload & (DMTIMER_TCLR_AR | 
                                                   DMTIMER_TCLR_CE));
    /* Enable the interrupt on overflow only */
    HWREG(timerRegs[timer] + DMTIMER_IRQENABLE_SET) = DMTIMER_INT_OVF_EN_FLAG;

}

void freezeTimer(int timer)
{
    /* Stop the timer */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) &= ~DMTIMER_TCLR_ST;
}

unsigned int getElapsedTime(int timer)
{
    /* Read the counter value from TCRR */
    return (HWREG(timerRegs[timer] + DMTIMER_TCRR)-initValues[timer])/24;
}

  
static void DMTimer2Isr(void)
{
    callbacks[0]();
    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_IT_FLAG);   
}

static void DMTimer3Isr(void)
{
    callbacks[1]();
    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_3_REGS, DMTIMER_INT_OVF_IT_FLAG);
}

static void DMTimer4Isr(void)
{
    callbacks[2]();
    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_4_REGS, DMTIMER_INT_OVF_IT_FLAG);
}


static void DMTimer6Isr(void)
{
    callbacks[3]();
    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_6_REGS, DMTIMER_INT_OVF_IT_FLAG);
}

static void DMTimer7Isr(void)
{
    callbacks[4]();
    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_7_REGS, DMTIMER_INT_OVF_IT_FLAG);    
}