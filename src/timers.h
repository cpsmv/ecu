 
/* SAMPLE USAGE!!!!!

int main(void)
{
    initTimers();
    configTimer(3,1000000, func1);
    configTimer(2,500000, func2);

    while(1);
}

*/

void initTimers();
void configTimer(int timer, unsigned int microseconds, void (*callback)());
