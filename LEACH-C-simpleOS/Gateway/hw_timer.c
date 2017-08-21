//
// Simple timers
//
// Use timer 1, running at full clock speed
//

#include "hw_timer.h"

#include <stdbool.h>
#include <avr/io.h>

// get CPU clock frequency

unsigned long hw_timer_cpuf() {
   return F_CPU;
}


// initialise timer 1

void hw_timer_init(void)
{

   // set timer counter 1 to be free running at the clock frequency

   TCCR1A = 0x00;
   TCCR1B = 0x01;
   TCCR1C = 0x00;
   TCNT1 = 0x0000;
 
   TIMSK1  = 0x00; // no interrupts

}

void hw_timer_start(unsigned short value)
{
   hw_timer_init();
   TCNT1 = value;
}

bool hw_timer_overflow()
{
   bool set=((TIFR1 & (1<<TOV1))!=0);
   if (set)
      TIFR1 = 1<<TOV1; // clear flag
   return set;
}

//
// return current time in clock cycles
//
unsigned short hw_timer_now()
{
   return TCNT1;
}

//
// return current time in us
//
unsigned long basetime=0;
unsigned long hw_timer_now_us()
{
   unsigned long temp = TCNT1 / (F_CPU/1000000);
   if (hw_timer_overflow()) {
      basetime += 65536 / (F_CPU/1000000);
   }
   temp += basetime;
   return temp;
}

//
// Wait in micro seconds
// Maximum hardware delay is 65535 ticks
// Therefore may have to wait for the hardware timer multiple times
//

void hw_timer_wait_micro(unsigned short delay)
{
   unsigned long ticks;

   ticks = delay * (F_CPU/1000000);
  
   while (ticks>65535) {
      hw_timer_start(0);
	  ticks = ticks - 65536;
      while (!hw_timer_overflow());
   }

   hw_timer_start(65535-(ticks&0xFFFF));
   while (!hw_timer_overflow());
}

// Wait in milliseconds

void hw_timer_wait_milli(unsigned short delay)
{
   while (delay>65) {
      hw_timer_wait_micro(65000);
	  delay = delay - 65;
  }

   hw_timer_wait_micro(delay);
}




