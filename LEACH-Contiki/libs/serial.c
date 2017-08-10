#include "serial.h"

#include <avr/io.h>

// setup the UART at the specified speed (baud rate)
// note: read the 128RFA1 datasheet - depending on the clock speed, only some
//       baud rates work accurately

static unsigned long serial_baud0, serial_baud1;

void serial_init(unsigned char port, unsigned long baud)
{
   baud = baud/10;
   baud = ((F_CPU/16)/baud)+5;
   baud = (baud/10)-1;
   if (port==0) {
	   UCSR0B = 0x00;        // disable UART0
	   UCSR0A = 0x0;         // clear status; initialise normal mode
	   UCSR0C = 0x06;        // asynch, no parity, 1 stop, 8 bits
	   UBRR0H = (baud >> 8) & 0x0F;   // baud high
	   UBRR0L = baud & 0xFF; // baud low
	   UCSR0B = (1<<RXEN0) | (1<<TXEN0); // enable rx and tx
	   serial_baud0 = baud;
   }
   else if (port==1) {
	   UCSR1B = 0x00;        // disable UART1
	   UCSR1A = 0x0;         // clear status; initialise normal mode
	   UCSR1C = 0x06;        // asynch, no parity, 1 stop, 8 bits
	   if (baud<1000000) {
		   UBRR1H = (baud >> 8) & 0x0F;   // baud high
		   UBRR1L = baud & 0xFF; // baud low
	   }
	   else if (baud==1000000) {
		   UBRR1H = 0;
		   UBRR1L = 0;
		   UCSR1A |= (1<<U2X1);
	   }		   
	   UCSR1B = (1<<RXEN1) | (1<<TXEN1); // enable rx and tx
	   serial_baud1 = baud;
   }	   
		   
}

// send a character to the UART

void serial_put(unsigned char port, unsigned char c)
{
   if (port==0) {
	   while ( !(UCSR0A & (1<<UDRE0)) );
	   UDR0 = c;
   }
   else if (port==1) {
	   while ( !(UCSR1A & (1<<UDRE1)) );
	   UDR1 = c;
	   // serial_puts(0, "Sending: ");
	   // while ( !(UCSR0A & (1<<UDRE0)) );
	   // UDR0 = c;
	   // serial_puts(0, "\n\r");
   }	   
}

// send a string to the UART

void serial_puts(unsigned char port, char *text)
{
   while (*text!=0)
      serial_put(port, *text++);
}

// check if input data is ready

bool serial_ready(unsigned char port)
{
	if (port==0)
		return ((UCSR0A & (1<<RXC0))!=0);
	else if (port==1)
		return ((UCSR1A & (1<<RXC1))!=0);
	else
		return 0;
}

// read a character

char serial_get(unsigned char port)
{
	while ( !serial_ready(port) );
	if (port==0)
		return UDR0;
	else if (port==1)
		return UDR1;
	else
		return 0x00;
}

// Send a break

void serial_break(unsigned char port)
{
	if (port==1) {
		UCSR1B &= ~(1<<TXEN1); // disable tx
		DDRB |= (1<<PB3); // set PB3 (TX2) as output
		PORTB &= ~(1<<PB3); // drop PB3
		timer_wait_milli(10); // long break
		UCSR1B |= (1<<TXEN1); // re-enable tx
	}
}
