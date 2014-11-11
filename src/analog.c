#include "hw_control_AM335x.h"
#include "consoleUtils.h"
#include "soc_AM335x.h"
#include "bbblack.h"
#include "interrupt.h"
#include "tsc_adc.h"
#include "mmu.h"
#include "cache.h"
#include "hw_cm_per.h"
#include "hw_types.h"
#include "hw_cm_wkup.h"

/******************************************************************************
**              INTERNAL MACRO DEFINITIONS
******************************************************************************/
#define  TSC_ADC_INSTANCE             SOC_ADC_TSC_0_REGS

#define  RESOL_X_MILLION              (439u)

#define  START_ADDR_DDR               (0x80000000)

#define  START_ADDR_DEV               (0x44000000)

#define  START_ADDR_OCMC              (0x40300000)

#define  NUM_SECTIONS_DDR             (512)

#define  NUM_SECTIONS_DEV             (960)

#define  NUM_SECTIONS_OCMC            (1)


/******************************************************************************
**              INTERNAL FUNCTION PROTOTYPES
******************************************************************************/
static void ADCIsr();
static void AnalogInit();
static void StepConfigure(unsigned int, unsigned int, unsigned int);
static void TSCADCModuleClkConfig(void);
static void TSCADCPinMuxSetUp(void)

/******************************************************************************
**              GLOBAL VARIABLE DEFINITIONS
******************************************************************************/
unsigned int sample1;
unsigned int sample2;
unsigned int val1;
unsigned int val2;

/* page tables start must be aligned in 16K boundary */
#ifdef __TMS470__
#pragma DATA_ALIGN(pageTable, MMU_PAGETABLE_ALIGN_SIZE);
static volatile unsigned int pageTable[MMU_PAGETABLE_NUM_ENTRY];

#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=MMU_PAGETABLE_ALIGN_SIZE
static volatile unsigned int pageTable[MMU_PAGETABLE_NUM_ENTRY];

#elif defined(gcc)
static volatile unsigned int pageTable[MMU_PAGETABLE_NUM_ENTRY]
 __attribute__((aligned(MMU_PAGETABLE_ALIGN_SIZE)));

#else
#error "Unsupported Compiler. \r\n"

#endif

/****************************************************************************/
/*             LOCAL FUNCTION DEFINITIONS                                   */
/****************************************************************************/

/*
** Function to setup MMU. This function Maps three regions ( 1. DDR
** 2. OCMC and 3. Device memory) and enables MMU.
*/
void MMUConfigAndEnable(void)
{
    /*
    ** Define DDR memory region of AM335x. DDR can be configured as Normal
    ** memory with R/W access in user/privileged modes. The cache attributes 
    ** specified here are, 
    ** Inner - Write through, No Write Allocate
    ** Outer - Write Back, Write Allocate
    */
    REGION regionDdr = {
                        MMU_PGTYPE_SECTION, START_ADDR_DDR, NUM_SECTIONS_DDR,
                        MMU_MEMTYPE_NORMAL_NON_SHAREABLE(MMU_CACHE_WT_NOWA,
                                                         MMU_CACHE_WB_WA),
                        MMU_REGION_NON_SECURE, MMU_AP_PRV_RW_USR_RW,
                        (unsigned int*)pageTable
                       };
    /*
    ** Define OCMC RAM region of AM335x. Same Attributes of DDR region given. 
    */
    REGION regionOcmc = {
                         MMU_PGTYPE_SECTION, START_ADDR_OCMC, NUM_SECTIONS_OCMC,
                         MMU_MEMTYPE_NORMAL_NON_SHAREABLE(MMU_CACHE_WT_NOWA,
                                                          MMU_CACHE_WB_WA),
                         MMU_REGION_NON_SECURE, MMU_AP_PRV_RW_USR_RW,
                         (unsigned int*)pageTable
                        };

    /*
    ** Define Device Memory Region. The region between OCMC and DDR is
    ** configured as device memory, with R/W access in user/privileged modes.
    ** Also, the region is marked 'Execute Never'.
    */
    REGION regionDev = {
                        MMU_PGTYPE_SECTION, START_ADDR_DEV, NUM_SECTIONS_DEV,
                        MMU_MEMTYPE_DEVICE_SHAREABLE,
                        MMU_REGION_NON_SECURE,
                        MMU_AP_PRV_RW_USR_RW  | MMU_SECTION_EXEC_NEVER,
                        (unsigned int*)pageTable
                       };

    /* Initialize the page table and MMU */
    MMUInit((unsigned int*)pageTable);

    /* Map the defined regions */
    MMUMemRegionMap(&regionDdr);
    MMUMemRegionMap(&regionOcmc);
    MMUMemRegionMap(&regionDev);

    /* Now Safe to enable MMU */
    MMUEnable((unsigned int*)pageTable);
}


int main(void)
{
    MMUConfigAndEnable();

    CacheEnable(CACHE_ALL);


    /* Initialize the UART console */
    ConsoleUtilsInit();

    /* Select the console type based on compile time check */
    ConsoleUtilsSetType(CONSOLE_UART);


    AnalogInit();

    while(1) {

	    val1 = (sample1 * RESOL_X_MILLION) / 1000;

	    ConsoleUtilsPrintf("Voltage sensed on the AN0 line : ");

	    ConsoleUtilsPrintf("%d", val1);

	    ConsoleUtilsPrintf("mV\r\n");

	    val2 = (sample2 * RESOL_X_MILLION) / 1000;

	    ConsoleUtilsPrintf("Voltage sensed on the AN1 line : ");

	    ConsoleUtilsPrintf("%d", val2);

	    ConsoleUtilsPrintf("mV\r\n");
	}

}


/* Configures the step */
void StepConfigure(unsigned int stepSel, unsigned int fifo,
                   unsigned int positiveInpChannel)
{
    /* Configure ADC to Single ended operation mode */
    TSCADCTSStepOperationModeControl(TSC_ADC_INSTANCE,
                                  TSCADC_SINGLE_ENDED_OPER_MODE, stepSel);

    /* Configure step to select Channel, refernce voltages */
    TSCADCTSStepConfig(TSC_ADC_INSTANCE, stepSel, TSCADC_NEGATIVE_REF_VSSA,
                    positiveInpChannel, TSCADC_NEGATIVE_INP_CHANNEL1, TSCADC_POSITIVE_REF_VDDA);

    /* XPPSW Pin is on, Which pull up the AN0 line*/
    TSCADCTSStepAnalogSupplyConfig(TSC_ADC_INSTANCE, TSCADC_XPPSW_PIN_ON, TSCADC_XNPSW_PIN_OFF,
                                TSCADC_YPPSW_PIN_OFF, stepSel);

    /* XNNSW Pin is on, Which pull down the AN1 line*/
    TSCADCTSStepAnalogGroundConfig(TSC_ADC_INSTANCE, TSCADC_XNNSW_PIN_ON, TSCADC_YPNSW_PIN_OFF,
                                TSCADC_YNNSW_PIN_OFF,  TSCADC_WPNSW_PIN_OFF, stepSel);

    /* select fifo 0 or 1*/
    TSCADCTSStepFIFOSelConfig(TSC_ADC_INSTANCE, stepSel, fifo);

    /* Configure ADC to one short mode */
    TSCADCTSStepModeConfig(TSC_ADC_INSTANCE, stepSel,  TSCADC_CONTINIOUS_SOFTWARE_ENABLED);
}

/* Reads the data from FIFO 0 and FIFO 1 */
static void ADCIsr()
{
    volatile unsigned int status;

    status = TSCADCIntStatus(TSC_ADC_INSTANCE);

    TSCADCIntStatusClear(TSC_ADC_INSTANCE, status);

    if(status & TSCADC_END_OF_SEQUENCE_INT)
    {
         /* Read data from fifo 0 */
         sample1 = TSCADCFIFOADCDataRead(TSC_ADC_INSTANCE, TSCADC_FIFO_0);

         /* Read data from fif 1*/
         sample2 = TSCADCFIFOADCDataRead(TSC_ADC_INSTANCE, TSCADC_FIFO_1);
    }
}


static void TSCADCModuleClkConfig(void)
{
    /* Configuring L3 Interface Clocks. */

    /* Writing to MODULEMODE field of CM_PER_L3_CLKCTRL register. */
    HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKCTRL) |=
          CM_PER_L3_CLKCTRL_MODULEMODE_ENABLE;

    /* Waiting for MODULEMODE field to reflect the written value. */
    while(CM_PER_L3_CLKCTRL_MODULEMODE_ENABLE !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKCTRL) &
           CM_PER_L3_CLKCTRL_MODULEMODE));

    /* Writing to MODULEMODE field of CM_PER_L3_INSTR_CLKCTRL register. */
    HWREG(SOC_CM_PER_REGS + CM_PER_L3_INSTR_CLKCTRL) |=
          CM_PER_L3_INSTR_CLKCTRL_MODULEMODE_ENABLE;

    /* Waiting for MODULEMODE field to reflect the written value. */
    while(CM_PER_L3_INSTR_CLKCTRL_MODULEMODE_ENABLE !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_L3_INSTR_CLKCTRL) &
           CM_PER_L3_INSTR_CLKCTRL_MODULEMODE));

    /* Writing to CLKTRCTRL field of CM_PER_L3_CLKSTCTRL register. */
    HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKSTCTRL) |=
          CM_PER_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    /* Waiting for CLKTRCTRL field to reflect the written value. */
    while(CM_PER_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKSTCTRL) &
           CM_PER_L3_CLKSTCTRL_CLKTRCTRL));

    /* Writing to CLKTRCTRL field of CM_PER_OCPWP_L3_CLKSTCTRL register. */
    HWREG(SOC_CM_PER_REGS + CM_PER_OCPWP_L3_CLKSTCTRL) |=
          CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    /*Waiting for CLKTRCTRL field to reflect the written value. */
    while(CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_OCPWP_L3_CLKSTCTRL) &
           CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL));

    /* Writing to CLKTRCTRL field of CM_PER_L3S_CLKSTCTRL register. */
    HWREG(SOC_CM_PER_REGS + CM_PER_L3S_CLKSTCTRL) |=
          CM_PER_L3S_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    /*Waiting for CLKTRCTRL field to reflect the written value. */
    while(CM_PER_L3S_CLKSTCTRL_CLKTRCTRL_SW_WKUP !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_L3S_CLKSTCTRL) &
           CM_PER_L3S_CLKSTCTRL_CLKTRCTRL));

    /* Checking fields for necessary values.  */

    /* Waiting for IDLEST field in CM_PER_L3_CLKCTRL register to be set to 0x0. */
    while((CM_PER_L3_CLKCTRL_IDLEST_FUNC << CM_PER_L3_CLKCTRL_IDLEST_SHIFT)!=
          (HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKCTRL) & CM_PER_L3_CLKCTRL_IDLEST));

    /*
    ** Waiting for IDLEST field in CM_PER_L3_INSTR_CLKCTRL register to attain the
    ** desired value.
    */
    while((CM_PER_L3_INSTR_CLKCTRL_IDLEST_FUNC <<
           CM_PER_L3_INSTR_CLKCTRL_IDLEST_SHIFT)!=
          (HWREG(SOC_CM_PER_REGS + CM_PER_L3_INSTR_CLKCTRL) &
           CM_PER_L3_INSTR_CLKCTRL_IDLEST));

    /*
    ** Waiting for CLKACTIVITY_L3_GCLK field in CM_PER_L3_CLKSTCTRL register to
    ** attain the desired value.
    */
    while(CM_PER_L3_CLKSTCTRL_CLKACTIVITY_L3_GCLK !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKSTCTRL) &
           CM_PER_L3_CLKSTCTRL_CLKACTIVITY_L3_GCLK));

    /*
    ** Waiting for CLKACTIVITY_OCPWP_L3_GCLK field in CM_PER_OCPWP_L3_CLKSTCTRL
    ** register to attain the desired value.
    */
    while(CM_PER_OCPWP_L3_CLKSTCTRL_CLKACTIVITY_OCPWP_L3_GCLK !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_OCPWP_L3_CLKSTCTRL) &
           CM_PER_OCPWP_L3_CLKSTCTRL_CLKACTIVITY_OCPWP_L3_GCLK));

    /*
    ** Waiting for CLKACTIVITY_L3S_GCLK field in CM_PER_L3S_CLKSTCTRL register
    ** to attain the desired value.
    */
    while(CM_PER_L3S_CLKSTCTRL_CLKACTIVITY_L3S_GCLK !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_L3S_CLKSTCTRL) &
          CM_PER_L3S_CLKSTCTRL_CLKACTIVITY_L3S_GCLK));


    /* Configuring registers related to Wake-Up region. */

    /* Writing to MODULEMODE field of CM_WKUP_CONTROL_CLKCTRL register. */
    HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CONTROL_CLKCTRL) |=
          CM_WKUP_CONTROL_CLKCTRL_MODULEMODE_ENABLE;

    /* Waiting for MODULEMODE field to reflect the written value. */
    while(CM_WKUP_CONTROL_CLKCTRL_MODULEMODE_ENABLE !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CONTROL_CLKCTRL) &
           CM_WKUP_CONTROL_CLKCTRL_MODULEMODE));

    /* Writing to CLKTRCTRL field of CM_PER_L3S_CLKSTCTRL register. */
    HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CLKSTCTRL) |=
          CM_WKUP_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    /*Waiting for CLKTRCTRL field to reflect the written value. */
    while(CM_WKUP_CLKSTCTRL_CLKTRCTRL_SW_WKUP !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CLKSTCTRL) &
           CM_WKUP_CLKSTCTRL_CLKTRCTRL));

    /* Writing to CLKTRCTRL field of CM_L3_AON_CLKSTCTRL register. */
    HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_L3_AON_CLKSTCTRL) |=
          CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    /*Waiting for CLKTRCTRL field to reflect the written value. */
    while(CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKTRCTRL_SW_WKUP !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_L3_AON_CLKSTCTRL) &
           CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKTRCTRL));

    /* Writing to MODULEMODE field of CM_WKUP_TSC_CLKCTRL register. */
    HWREG(SOC_CM_WKUP_REGS + CM_WKUP_ADC_TSC_CLKCTRL) |=
          CM_WKUP_ADC_TSC_CLKCTRL_MODULEMODE_ENABLE;

    /* Waiting for MODULEMODE field to reflect the written value. */
    while(CM_WKUP_ADC_TSC_CLKCTRL_MODULEMODE_ENABLE !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_ADC_TSC_CLKCTRL) &
           CM_WKUP_ADC_TSC_CLKCTRL_MODULEMODE));

    /* Verifying if the other bits are set to required settings. */

    /*
    ** Waiting for IDLEST field in CM_WKUP_CONTROL_CLKCTRL register to attain
    ** desired value.
    */
    while((CM_WKUP_CONTROL_CLKCTRL_IDLEST_FUNC <<
           CM_WKUP_CONTROL_CLKCTRL_IDLEST_SHIFT) !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CONTROL_CLKCTRL) &
           CM_WKUP_CONTROL_CLKCTRL_IDLEST));

    /*
    ** Waiting for CLKACTIVITY_L3_AON_GCLK field in CM_L3_AON_CLKSTCTRL
    ** register to attain desired value.
    */
    while(CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKACTIVITY_L3_AON_GCLK !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_L3_AON_CLKSTCTRL) &
           CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKACTIVITY_L3_AON_GCLK));

    /*
    ** Waiting for IDLEST field in CM_WKUP_L4WKUP_CLKCTRL register to attain
    ** desired value.
    */
    while((CM_WKUP_L4WKUP_CLKCTRL_IDLEST_FUNC <<
           CM_WKUP_L4WKUP_CLKCTRL_IDLEST_SHIFT) !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_L4WKUP_CLKCTRL) &
           CM_WKUP_L4WKUP_CLKCTRL_IDLEST));

    /*
    ** Waiting for CLKACTIVITY_L4_WKUP_GCLK field in CM_WKUP_CLKSTCTRL register
    ** to attain desired value.
    */
    while(CM_WKUP_CLKSTCTRL_CLKACTIVITY_L4_WKUP_GCLK !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CLKSTCTRL) &
           CM_WKUP_CLKSTCTRL_CLKACTIVITY_L4_WKUP_GCLK));

    /*
    ** Waiting for CLKACTIVITY_L4_WKUP_AON_GCLK field in CM_L4_WKUP_AON_CLKSTCTRL
    ** register to attain desired value.
    */
    while(CM_WKUP_CM_L4_WKUP_AON_CLKSTCTRL_CLKACTIVITY_L4_WKUP_AON_GCLK !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_L4_WKUP_AON_CLKSTCTRL) &
           CM_WKUP_CM_L4_WKUP_AON_CLKSTCTRL_CLKACTIVITY_L4_WKUP_AON_GCLK));

    /*
    ** Waiting for CLKACTIVITY_ADC_FCLK field in CM_WKUP_CLKSTCTRL
    ** register to attain desired value.
    */
    while(CM_WKUP_CLKSTCTRL_CLKACTIVITY_ADC_FCLK !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CLKSTCTRL) &
           CM_WKUP_CLKSTCTRL_CLKACTIVITY_ADC_FCLK));

    /*
    ** Waiting for IDLEST field in CM_WKUP_ADC_TSC_CLKCTRL register to attain
    ** desired value.
    */
    while((CM_WKUP_ADC_TSC_CLKCTRL_IDLEST_FUNC <<
           CM_WKUP_ADC_TSC_CLKCTRL_IDLEST_SHIFT) !=
          (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_ADC_TSC_CLKCTRL) &
           CM_WKUP_ADC_TSC_CLKCTRL_IDLEST));
}

static void TSCADCPinMuxSetUp(void)
{

    HWREG( SOC_CONTROL_REGS + CONTROL_CONF_AIN0) = CONTROL_CONF_AIN0_CONF_AIN0_RXACTIVE;

    HWREG( SOC_CONTROL_REGS + CONTROL_CONF_AIN1) = CONTROL_CONF_AIN1_CONF_AIN1_RXACTIVE;

    HWREG( SOC_CONTROL_REGS +  CONTROL_CONF_AIN2)= CONTROL_CONF_AIN2_CONF_AIN2_RXACTIVE;

    HWREG( SOC_CONTROL_REGS + CONTROL_CONF_AIN3) = CONTROL_CONF_AIN3_CONF_AIN3_RXACTIVE;

    HWREG( SOC_CONTROL_REGS + CONTROL_CONF_AIN4) = CONTROL_CONF_AIN4_CONF_AIN4_RXACTIVE;

    HWREG( SOC_CONTROL_REGS + CONTROL_CONF_AIN5) = CONTROL_CONF_AIN5_CONF_AIN5_RXACTIVE;

    HWREG( SOC_CONTROL_REGS + CONTROL_CONF_AIN6) = CONTROL_CONF_AIN6_CONF_AIN6_RXACTIVE;

    HWREG( SOC_CONTROL_REGS + CONTROL_CONF_AIN7) = CONTROL_CONF_AIN7_CONF_AIN7_RXACTIVE;

    HWREG( SOC_CONTROL_REGS + CONTROL_CONF_VREFP)= CONTROL_CONF_VREFP_CONF_VREFP_RXACTIVE;

    HWREG( SOC_CONTROL_REGS +  CONTROL_CONF_VREFN)= CONTROL_CONF_VREFN_CONF_VREFN_RXACTIVE;
}



void AnalogInit() {

	IntMasterIRQEnable();

    /* Initialize the ARM Interrupt Controller.*/
    IntAINTCInit();

	IntRegister(SYS_INT_ADC_TSC_GENINT, ADCIsr);

    IntPrioritySet(SYS_INT_ADC_TSC_GENINT, 5, AINTC_HOSTINT_ROUTE_IRQ);

    IntSystemEnable(SYS_INT_ADC_TSC_GENINT);

    /* Enable the clock for touch screen */
    TSCADCModuleClkConfig();

    TSCADCPinMuxSetUp();

    /* Configures ADC to 3Mhz */
    TSCADCConfigureAFEClock(TSC_ADC_INSTANCE, 24000000, 3000000);

    /* Enable Transistor bias */
    TSCADCTSTransistorConfig(TSC_ADC_INSTANCE, TSCADC_TRANSISTOR_ENABLE);

    TSCADCStepIDTagConfig(TSC_ADC_INSTANCE, 1);

    /* Disable Write Protection of Step Configuration regs*/
    TSCADCStepConfigProtectionDisable(TSC_ADC_INSTANCE);

    /* Configure step 1 for channel 1(AN0)*/
    StepConfigure(0, TSCADC_FIFO_0, TSCADC_POSITIVE_INP_CHANNEL1);

    /* Configure step 2 for channel 2(AN1)*/
    StepConfigure(1, TSCADC_FIFO_1, TSCADC_POSITIVE_INP_CHANNEL2);

    /* General purpose inputs */
    TSCADCTSModeConfig(TSC_ADC_INSTANCE, TSCADC_GENERAL_PURPOSE_MODE);

    /* Enable step 1 */
    TSCADCConfigureStepEnable(TSC_ADC_INSTANCE, 1, 1);

    /* Enable step 2 */
    TSCADCConfigureStepEnable(TSC_ADC_INSTANCE, 2, 1);

    /* Clear the status of all interrupts */
    TSCADCIntStatusClear(TSC_ADC_INSTANCE, 0x7FF);
    TSCADCIntStatusClear(TSC_ADC_INSTANCE ,0x7FF);
    TSCADCIntStatusClear(TSC_ADC_INSTANCE, 0x7FF);

    /* End of sequence interrupt is enable */
    TSCADCEventInterruptEnable(TSC_ADC_INSTANCE, TSCADC_END_OF_SEQUENCE_INT);

    /* Enable the TSC_ADC_SS module*/
    TSCADCModuleStateSet(TSC_ADC_INSTANCE, TSCADC_MODULE_ENABLE);
}