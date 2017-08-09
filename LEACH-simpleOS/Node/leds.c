#include "leds.h"

#include <avr/io.h>

//
// Internal constants
//

// Note: the LEDs are connected to pins 2,3,4 of Port E
//       they are wired so that 0=on, 1=off

// The recommended way to set bit x on a port y is
//    PORTy != (1<<x)
// The recommended way to clear bit x on a port y is
//    PORTy &= ~(1<<x)

#define LED1_bit 2
#define LED2_bit 3
#define LED3_bit 4

#define LED1_OFF  PORTE |= (1<<LED1_bit)
#define LED1_ON   PORTE &= ~(1<<LED1_bit)

#define LED2_OFF  PORTE |= (1<<LED2_bit)
#define LED2_ON   PORTE &= ~(1<<LED2_bit)

#define LED3_OFF  PORTE |= (1<<LED3_bit)
#define LED3_ON   PORTE &= ~(1<<LED3_bit)

#define LED1   ((PINE & (1<<LED1_bit)))
#define LED2   ((PINE & (1<<LED2_bit)))
#define LED3   ((PINE & (1<<LED3_bit)))

#define LED1_OUT  DDRE |= (1<<LED1_bit)
#define LED2_OUT  DDRE |= (1<<LED2_bit)
#define LED3_OUT  DDRE |= (1<<LED3_bit)

//
// Initialise the LEDs
// The ATMega pins connected to the LEDs need to be set for "output"
//

void leds_init(void)
{
   LED1_OUT;
   LED2_OUT;
   LED3_OUT;

   LED1_OFF;
   LED2_OFF;
   LED3_OFF;
}

// Turn on a LED

void leds_on(unsigned char led)
{
   if (led==1)
      LED1_ON;
   else if (led==2)
      LED2_ON;
   else if (led==3)
      LED3_ON;
}

// Turn off a LED

void leds_off(unsigned char led)
{
   if (led==1)
      LED1_OFF;
   else if (led==2)
      LED2_OFF;
   else if (led==3)
      LED3_OFF;
}
