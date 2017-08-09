#include <stdbool.h>


//#define F_CPU 16000000

#ifndef F_CPU
#define F_CPU 16000000
#endif /* F_CPU */

#ifndef TIMER_H

#define TIMER_H

//void timer_init(void);
//void timer_start(unsigned short value);
bool timer_overflow();
unsigned short timer_now();
unsigned long timer_now_us();
void timer_wait_micro(unsigned short delay);
void timer_wait_milli(unsigned short delay);
#endif /*TIMER_H */
