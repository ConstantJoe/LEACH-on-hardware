//
// Simple timers
//
// Use timer 1, running at full clock speed
//

#include "timer.h"

#include <stdbool.h>
#include <avr/io.h>

// initialise timer 1

/*void timer_init(void)
{

   // set timer counter 1 to be free running at the clock frequency

   TCCR1A = 0x00;
   TCCR1B = 0x01;
   TCCR1C = 0x00;
   TCNT1 = 0x0000;
   TIMSK1  = 0x00; // no interrupts

}*/

/*void timer_start(unsigned short value)
{
   timer_init();
   TCNT1 = value;
}*/

bool timer_overflow()
{
   bool set=((TIFR1 & (1<<TOV1))!=0);
   if (set)
      TIFR1 = 1<<TOV1; // clear flag
   return set;
}

//
// return current time in clock cycles
//
unsigned short timer_now()
{
   return TCNT1;
}

//
// return current time in us
//
unsigned long timer_now_us()
{
   unsigned short temp = TCNT1 / (F_CPU/1000000);
   return temp;
}

//
// Wait in micro seconds
// Maximum hardware delay is 65535 ticks
// Therefore may have to wait for the hardware timer multiple times
//

void timer_wait_micro(unsigned short delay)
{
   unsigned long ticks;

   ticks = delay * (F_CPU/1000000);
  
   while (ticks>65535) {
      timer_start(0);
	  ticks = ticks - 65535;
      while (!timer_overflow());
   }

   timer_start(65535-(ticks&0xFFFF));
   while (!timer_overflow());
}

// Wait in milliseconds

void timer_wait_milli(unsigned short delay)
{
   while (delay>65) {
      timer_wait_micro(65000);
	  delay = delay - 65;
  }

   timer_wait_micro(delay);
}
