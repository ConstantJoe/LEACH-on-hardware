#include <stdbool.h>

//#define F_CPU 16000000

void timer_init(void);
void timer_start(unsigned short value);
bool timer_overflow();
unsigned short timer_now();
unsigned long timer_now_us();
void timer_wait_micro(unsigned short delay);
void timer_wait_milli(unsigned short delay);

