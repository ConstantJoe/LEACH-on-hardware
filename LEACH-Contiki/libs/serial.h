#include <stdbool.h>

//#define F_CPU 16000000

void serial_init(unsigned char port, unsigned long baud);
void serial_put(unsigned char port, unsigned char c);
void serial_puts(unsigned char port, char *text);
bool serial_ready(unsigned char port);
char serial_get(unsigned char port);
void serial_break(unsigned char port);
