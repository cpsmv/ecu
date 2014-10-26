#include "soc_AM335x.h"
#include "beaglebone.h"
#include "interrupt.h"
#include "dmtimer.h"
#include "timers.h"
#include "cpu.h"
#include "pin_mux.h"


// #define TIMER_CONVERT_US(microsec) 	((0xFFFFFFFFu-(microsec*26u)))
#define TIMER_CONVERT_US(microsec)  ((0xFFFFFFFFu-(microsec*24u)))

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
	CPUirqe();
	IntAINTCInit();
}

void configTimer(int timer, unsigned int microseconds, unsigned int autoReload, void (*callback)()) 
{
	timerClkConfigs[timer]();
	IntRegister(timerInts[timer],timerISRs[timer]);

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

    /* Wait for previous write to complete */
    //DMTimerWaitForWrite(DMTIMER_WRITE_POST_TCLR, timerRegs[timer]);

    /* Disable Pre-scaler clock */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) &= ~DMTIMER_TCLR_PRE;


	// /* Reset the DMTimer module */
    // HWREG(timerRegs[timer] + DMTIMER_TIOCP_CFG) |= DMTIMER_TIOCP_CFG_SOFTRESET;

    // while(DMTIMER_TIOCP_CFG_SOFTRESET == (HWREG(timerRegs[timer] + DMTIMER_TIOCP_CFG)
    //                                        & DMTIMER_TIOCP_CFG_SOFTRESET));

    
    //DMTimerPostedModeConfigure(timerRegs[timer], DMTIMER_POSTED_INACTIVE);

    /* Load the counter with the initial count value */
    	/* Wait for previous write to complete */
    //DMTimerWaitForWrite(DMTIMER_WRITE_POST_TCRR, timerRegs[timer]);

    /* Set the counter value */
    HWREG(timerRegs[timer] + DMTIMER_TCRR) = initValues[timer]=TIMER_CONVERT_US(microseconds);

    /* Load the load register with the reload count value */
    /* Wait for previous write to complete */
    //DMTimerWaitForWrite(DMTIMER_WRITE_POST_TLDR, timerRegs[timer]);

    /* Load the register with the re-load value */
    HWREG(timerRegs[timer] + DMTIMER_TLDR) = TIMER_CONVERT_US(microseconds);

    /* Configure the DMTimer for Auto-reload mode or One-shot mode */
    /* Wait for previous write to complete */
    //DMTimerWaitForWrite(DMTIMER_WRITE_POST_TCLR, timerRegs[timer]);

    /* Clear the AR and CE field of TCLR */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) &= ~(DMTIMER_TCLR_AR | DMTIMER_TCLR_CE);

    /* Wait for previous write to complete */
    //DMTimerWaitForWrite(DMTIMER_WRITE_POST_TCLR, timerRegs[timer]);

    /* Set the timer mode in TCLR register */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) |= (autoReload & (DMTIMER_TCLR_AR | 
                                                   DMTIMER_TCLR_CE));


    HWREG(timerRegs[timer] + DMTIMER_IRQENABLE_SET) = (DMTIMER_INT_OVF_EN_FLAG & 
                                           (DMTIMER_IRQENABLE_SET_TCAR_EN_FLAG |
                                            DMTIMER_IRQENABLE_SET_OVF_EN_FLAG | 
                                            DMTIMER_IRQENABLE_SET_MAT_EN_FLAG));

    //DMTimerWaitForWrite(DMTIMER_WRITE_POST_TCLR, timerRegs[timer]);

    /* Start the timer */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) |= DMTIMER_TCLR_ST;
}

void freezeTimer(int timer)
{
    /* Wait for previous write to complete */
    //DMTimerWaitForWrite(DMTIMER_WRITE_POST_TCLR, timerRegs[timer]);

    /* Stop the timer */
    HWREG(timerRegs[timer] + DMTIMER_TCLR) &= ~DMTIMER_TCLR_ST;
}

unsigned int getElapsedTime(int timer)
{
    /* Wait for previous write to complete */
    //DMTimerWaitForWrite(DMTIMER_WRITE_POST_TCRR, timerRegs[timer]);

    /* Read the counter value from TCRR */
    return (HWREG(timerRegs[timer] + DMTIMER_TCRR)-initValues[timer])/24;
}

  
static void DMTimer2Isr(void)
{
    /* Disable the DMTimer interrupts */
    //DMTimerIntDisable(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_IT_FLAG);

    callbacks[0]();

    /* Enable the DMTimer interrupts */
    //DMTimerIntEnable(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);
}

static void DMTimer3Isr(void)
{
    /* Disable the DMTimer interrupts */
    //DMTimerIntDisable(SOC_DMTIMER_3_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_3_REGS, DMTIMER_INT_OVF_IT_FLAG);

    callbacks[1]();

    /* Enable the DMTimer interrupts */
    //DMTimerIntEnable(SOC_DMTIMER_3_REGS, DMTIMER_INT_OVF_EN_FLAG);
}

static void DMTimer4Isr(void)
{
    /* Disable the DMTimer interrupts */
    //DMTimerIntDisable(SOC_DMTIMER_4_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_4_REGS, DMTIMER_INT_OVF_IT_FLAG);

    callbacks[2]();

    /* Enable the DMTimer interrupts */
    //DMTimerIntEnable(SOC_DMTIMER_4_REGS, DMTIMER_INT_OVF_EN_FLAG);
}


static void DMTimer6Isr(void)
{
    /* Disable the DMTimer interrupts */
    //DMTimerIntDisable(SOC_DMTIMER_6_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_6_REGS, DMTIMER_INT_OVF_IT_FLAG);

    callbacks[3]();

    /* Enable the DMTimer interrupts */
    //DMTimerIntEnable(SOC_DMTIMER_6_REGS, DMTIMER_INT_OVF_EN_FLAG);
}

static void DMTimer7Isr(void)
{
    /* Disable the DMTimer interrupts */
    //DMTimerIntDisable(SOC_DMTIMER_7_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_7_REGS, DMTIMER_INT_OVF_IT_FLAG);

    callbacks[4]();

    /* Enable the DMTimer interrupts */
    //DMTimerIntEnable(SOC_DMTIMER_7_REGS, DMTIMER_INT_OVF_EN_FLAG);
}