//
// AVR C library
//
#include <avr/io.h>
//
// Standard C include files
//
#include <stdbool.h>
#include <stdio.h>
#include <stdio.h>
//
// You MUST include app.h and implement every function declared
//
#include "app.h"
//
// Include the header files for the various required libraries
//
#include "simple_os.h"
#include "button.h"
#include "leds.h"
#include "radio.h"
#include "serial.h"
#include "hw_timer.h"
#include "app.h"

#include "Ctk_spi.h"
#include "sx1272.h"

//
// Global Variables
//
static timer timer1;

int q = 0;
//
// App init function
//
void application_start()
{	

	//PB0 = SSN
	//PB1 = SCK
	//PB2 = MOSI
	//PB3 = MISO
	//PB4 = OC2A
	//PB5 = OC1A
	//PB6 = OC1B
	//PB7 = OC0A

	leds_init();
	button_init();

	//leds_on(LED_ORANGE);
	spix_load_lib(); 


	sx1272_init();
	sx1272_config0();

	serial_init(19200);
	timer_init(&timer1, TIMER_MILLISECONDS, 10, 5000); 
	timer_start(&timer1);

	serial_puts((char *) "serial inited\r\n");
}

void application_timer_tick(timer *t){
	leds_off(LED_GREEN);
	leds_off(LED_RED);
	leds_off(LED_ORANGE);
	serial_puts((char *) "timer tick\r\n");
}

void application_radio_rx_msg(unsigned short dst, unsigned short src, int len, unsigned char *msgdata){}

void application_radio_tx_done(){}

void application_button_pressed(){

	unsigned char data[16] = {0xFF, 0xFE, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80,
	                          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFD, 0xFF};

	int ret;

	if(q==0){
		ret = sx1272_force_RxTimeout();	
	
		if(ret == 1){
			//TxDone
			leds_on(LED_GREEN);
		}
		else if(ret == 0){
			//neither
			leds_on(LED_RED);
		}
		q++;
		serial_puts((char *) "forced RxTimeout to fire.\r\n");
	}
	else if(q==1){
		sx1272_set_TX();
		ret = sx1272_transmit(data, 16); // write data to Tx FIFO*/
	
		if(ret == 1){
			//TxDone
			leds_on(LED_ORANGE);
		}
		else if(ret == 0){
			//neither
			leds_on(LED_RED);
		}
		q--;
		serial_puts((char *) "sent a message.\r\n");
	}
	
}
	
void application_button_released(){}
