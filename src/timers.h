 
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

void initTimers();
void configTimer(int timer, unsigned int microseconds, unsigned int autoReload, void (*callback)());
