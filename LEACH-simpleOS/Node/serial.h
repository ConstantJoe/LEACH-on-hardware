#include <stdbool.h>
#include "timer.h"
//#define F_CPU 16000000

#ifndef F_CPU
#define F_CPU 16000000
#endif /* F_CPU */

void serial_init(unsigned long baud);
void serial_put(unsigned char c);
void serial_puts(char *text);
bool serial_ready();
char serial_get();
unsigned char serial_is_init();
