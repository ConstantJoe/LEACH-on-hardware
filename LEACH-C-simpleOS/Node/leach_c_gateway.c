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

#define NUMNODES 					100

//TODO: ack packets

// Buffer for transmitting radio packets
unsigned char tx_buffer[RADIO_MAX_LEN];
bool tx_buffer_inuse=false; // Check false and set to true before sending a message. Set to false in tx_done

int node_id = 0x0001;

int start = 1;

DataPacket dp;

int num_dead = 0;
int area_height = 100;
int area_width = 100;
int sink_x = 50;
int sink_y = 50;

int k;
int counted = 0;
/*
	This is just for the first round, to know how many messages to expect to come in.
*/
double clusterOptimum()
{
	//TODO: can't assumed number of nodes and location of sink will be known- see how its done in the paper
	double dBS = sqrt(pow(sink_x - area_width, 2) + pow(sink_y - area_height, 2));
	int n = NUMNODES - num_dead;
	double m = sqrt(area_height * area_width);

	float x = e_freespace / e_multipath;

	double kopt = sqrt(n) / sqrt(2*M_PI) * sqrt(x) * m / pow(dBS,2);
	kopt = round(kopt);
	return kopt;
}


//
// App init function
//
void application_start()
{
	// initialise required services
	radio_init(node_id);

	k = clusterOptimum();
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
	counted++;

	if(counted <= k){
		
		//collect data (including node data)	
		dp.type = msgdata[0];
		dp.src_id = msgdata[1];
		dp.data = msgdata[3];

		//TODO: reformat dp to include node data.
		//TODO: save that data here
		//acks will have to be included in this as well. 
		printf("Rx %d bytes: [%04x][%04x]\n", len, dst, src);

		if(counted == k){
			//simulated annealing
			//send back packets 
		}
	}



	//TODO: send an ack?

}

void application_radio_tx_done()
{
	tx_buffer_inuse = false;
}

void application_button_pressed(){}

void application_button_released(){}
