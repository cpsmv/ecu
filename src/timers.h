 
/* SAMPLE USAGE!!!!!

int main(void)
{
    initTimers();
    configTimer(3,1000000, func1);
    configTimer(2,500000, func2);

    while(1);
}

*/

#define TIMER_AUTORELOAD_ENABLE 	(0x00000002u)
#define TIMER_AUTORELOAD_DISABLE 	(0x00000000u)

enum TIMER
{
	TIMER0,
	TIMER1,
	TIMER2,
	TIMER3,
	TIMER4
};

void initTimers();
void configTimer(int timer, unsigned int microseconds, unsigned int autoReload, void (*callback)());
void freezeTimer(int timer);
unsigned int getElapsedTime(int timer);
