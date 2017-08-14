#include "button.h"

#include <avr/io.h>

#define BUTTON_BIT 5
#define BUTTON_MASK (1<<BUTTON_BIT)

void button_init()
{
	DDRE &= ~(1<<BUTTON_BIT);
	PORTE |= (1<<BUTTON_BIT); // enable pullup resistor
}

bool button_pressed()
{
	return ((PINE&BUTTON_MASK)==0);
}

bool button_released()
{
   return (!button_pressed());
}
