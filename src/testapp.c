#include "consoleUtils.h"
#include "timers.h"
#include "gpio_v2.h"
#include "soc_AM335x.h"
#include "beaglebone.h"

#define GPIO_INSTANCE_ADDRESS           (SOC_GPIO_1_REGS)
#define GPIO_INSTANCE_PIN_NUMBER        (13)

/*
	in this example, TIMER1's ISR
		1. freezes TIMER0,
		2. prints TIMER0's elapsed time, 333333us - the time it takes to run configTimer, 
		3. and restarts TIMER0
*/

unsigned int high = 1;
unsigned int length;

void func1()
{

}

void func2() 
{	
	// GPIOPinWrite(GPIO_INSTANCE_ADDRESS,
 //                     GPIO_INSTANCE_PIN_NUMBER,
 //                     high);
	// high = !high;
	freezeTimer(TIMER0);
	length = getElapsedTime(TIMER0);
	runTimer(TIMER0, 1000000);

}

int main() {
	int i;
	// GPIO1ModuleClkConfig();
	// GPIO1PinMuxSetup(GPIO_INSTANCE_PIN_NUMBER);

	// GPIOModuleEnable(GPIO_INSTANCE_ADDRESS);

 //    /* Resetting the GPIO module. */
 //    GPIOModuleReset(GPIO_INSTANCE_ADDRESS);

 //    /* Setting the GPIO pin as an output pin. */
 //    GPIODirModeSet(GPIO_INSTANCE_ADDRESS,
 //                   GPIO_INSTANCE_PIN_NUMBER,
 //                   GPIO_DIR_OUTPUT);

	ConsoleUtilsInit();

	ConsoleUtilsSetType(CONSOLE_UART);

	initTimers();
	
	configTimer(TIMER0, TIMER_AUTORELOAD_DISABLE, func1);
	configTimer(TIMER1, TIMER_AUTORELOAD_ENABLE, func2);

	runTimer(TIMER1, 100000);

	while(1) {
		for(i=0;i<10000;i++);
		ConsoleUtilsPrintf("%d\n",length);
	}
}	