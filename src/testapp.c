#include "consoleUtils.h"
#include "timers.h"

/*
	in this example, TIMER1's ISR
		1. freezes TIMER0,
		2. prints TIMER0's elapsed time, 333333us (TIMER1's interrupt rate), 
		3. and restarts TIMER0
*/
		
void func1() 
{
	ConsoleUtilsPrintf("1");
}

void func2() 
{	
	freezeTimer(0);
	ConsoleUtilsPrintf("%d\n", getElapsedTime(1));
	configTimer(TIMER0, 1000000, TIMER_AUTORELOAD_DISABLE, func1);
}

int main() {

	ConsoleUtilsInit();

	ConsoleUtilsSetType(CONSOLE_UART);

	initTimers();
	configTimer(TIMER0, 1000000, TIMER_AUTORELOAD_DISABLE, func1);
	configTimer(TIMER1, 333333, TIMER_AUTORELOAD_ENABLE, func2);

	while(1);
}