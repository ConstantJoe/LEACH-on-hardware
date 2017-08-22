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
#include "leach_c_gateway.h"
#include "message_structs.h"
#include "simulated_annealing.h"

static timer timer1;

#define NUMNODES 					100




//
// App init function
//
void application_start()
{
	// initialise required services
	radio_init(node_id);

	k = cluster_optimum();
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
	ep.type = msgdata[0];

	if(ep.type == P_ENERGYPACKET){
		ep.numNodes = msgdata[1];
	
		int i;
		for(i=0; i<ep.num_nodes;i++){
			//NOTE: this works based on the max number of nodes per cluster being 15, and using 16bit numbers for energy, locX, locY
			//		might change.
			int n_id = msgdata[3+i*2]; //get node id
			energies[n_id] = msgdata[33+i*2];
			loc_xs[n_id] = msgdata[63+i*2];
			loc_ys[n_id] = msgdata[93+i*2];
		}

		counted += ep.num_nodes;

		//send an ack back
		ea.type = P_ENERGYACK;
		ea.tdma_slot = tdma_v;

		tdma_v++;

		memcpy(&tx_buffer, &ea, sizeof(EnergyAck));
		radio_send(tx_buffer, sizeof(EnergyAck), src);
	}
	else if(ep.type == P_DATAPACKET){
		dp.type = msgdata[0];
		dp.src_id = msgdata[1];
		dp.data = msgdata[3];
		dp.finished = msgdata[5];

		// data is dummy, don't actually have to save it.

		if(counted >= NUMNODES){
			//data from all has arrived, we can now perform simulated annealing.
			//put results into FormationPacket fp
			simulatedAnnealing(k);
		}

		memcpy(&tx_buffer, &fp, sizeof(FormationPacket));
		radio_send(tx_buffer, sizeof(FormationPacket), 0xff); //broadcast
	}
}

void application_radio_tx_done()
{
	tx_buffer_inuse = false;
}

void application_button_pressed(){}

void application_button_released(){}
