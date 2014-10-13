#include "consoleUtils.h"
#include "timers.h"

void func1() {ConsoleUtilsPrintf("1");}
void func2() {ConsoleUtilsPrintf("2");}

int main() {

	ConsoleUtilsInit();

	ConsoleUtilsSetType(CONSOLE_UART);

	initTimers();
	configTimer(0, 1000000, func1);
	configTimer(1, 333333, func2);

	while(1);
}