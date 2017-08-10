//
// AVR C library
//
#include <avr/io.h>
#include <math.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "simple_os.h"
#include "button.h"
#include "leds.h"
#include "radio.h"
#include "serial.h"
#include "message_structs.h"

static timer timer1;

//TODO: ack packets

// Buffer for transmitting radio packets
unsigned char tx_buffer[RADIO_MAX_LEN];
bool tx_buffer_inuse=false; // Check false and set to true before sending a message. Set to false in tx_done

int node_id = 0x0001;

DataPacket dp;

//
// App init function
//
void application_start()
{
	// initialise required services
	radio_init(node_id);
}

//
// Timer tick handler
//
void application_timer_tick(timer *t){}

//
// This function is called whenever a radio message is received
// You must copy any data you need out of the packet - as 'msgdata' will be overwritten by the next message
//
void application_radio_rx_msg(unsigned short dst, unsigned short src, int len, unsigned char *msgdata)
{
	//is this it? Seems too easy

	dp.type = msgdata[0];
	dp.src_id = msgdata[1];
	dp.data = msgdata[3];
	//acks will have to be included in this as well. 
	printf("Rx %d bytes: [%04x][%04x]\n", len, dst, src);

	//TODO: send an ack?

}

void application_radio_tx_done()
{
	tx_buffer_inuse = false;
}

void application_button_pressed(){}

void application_button_released(){}
