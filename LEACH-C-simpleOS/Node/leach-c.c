//
// AVR C library
//
#include <avr/io.h>
#include <math.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message_structs.h"
#include "app.h"
#include "simple_os.h"
#include "button.h"
#include "leds.h"
#include "radio.h"
#include "serial.h"
#include "leach.h"

//TODO: 
//		finish off the TODOs below
//		write the gateway version
//		adjust power of node transceiver based on RSSI
//		RFA1 supports DSSS - find out how it works
//		cluster optimum - see the version in the paper
//		sleep mode added in
//		ack packets
//		general cleanup and comments
//		deal with time drift

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
	radio_init(node_id, 0);
	timer_init(&timer1, TIMER_MILLISECONDS, 30000, 250); //TODO: time between ticks might be changed
}

//
// Timer tick handler
//
void application_timer_tick(timer *t)
{
	if(state == S_START){
		round_no++;
		rounds_since_ch++;

		//the first round of LEACH-C is normal LEACH - cluster heads are chosen in a distributed way
		if(start){
			start = 0;
			k = clusterOptimum();
			//set C(i)
			// if a node has been a CH in the most recent r%(N/k) rounds, then C(i) = 0. Else C(i) = 1.
			double c_i;
			int r;

			if(rounds_since_ch > NUMNODES/k){
				//can be a CH
				//Probability it is chosen is (k)/(N-k*(r%N/k))

				int nodes_per_cluster = (int) NUMNODES / k;

				c_i = (k)/(NUMNODES-k*(round_no%nodes_per_cluster));
				c_i *= 100;

				r = rand()%100;

				if(c_i > r){
					role == 'C';
				}
				else{
					role == 'N';
				}
			}

			if(role == 'C'){
				// send ADV message
				ad.data = 0xffff;
				ad.src_id = node_id;
				ad.type = P_ADVERTISEMENT;

				//Basic CSMA:
				int r;
				while(!CCA_STATUS){
					//as long as clear channel assessment says the channel is busy
					hw_timer_wait_milli(rand()%100); //TODO: this might be too long
				}

				memcpy(&tx_buffer, &ad, sizeof(Advertisement));
				radio_send(tx_buffer, sizeof(Advertisement), 0xFF); //broadcast

				state = S_WAIT_FOR_JOIN_REQUESTS;
				rounds_since_ch = 0;

				time_holder = hw_timer_now_us(); 
			}
			else if(role == 'N'){
				state = S_WAIT_FOR_ADVERTISEMENTS;
				time_holder = hw_timer_now_us();
			}
		}
		else
		{
			//if its not the first round we can assume that the node has a cluster assigned already (so no advertisements needed)
			// cluster heads wake up, regular nodes send to their assigned CH according to their TDMA slot.
			// TODO: right here nodes send data AND their location data, energy level in one packet to the gateway. 
			if(role == 'N'){
				hw_timer_wait_milli(100*tdma_slot);
				dp.data = 0xffff;
				dp.src_id = node_id;
				dp.type = P_DATAPACKET;

				memcpy(&tx_buffer, &dp, sizeof(DataPacket));
				radio_send(tx_buffer, sizeof(DataPacket), ch_id);

				state = S_WAIT_FOR_FORMATION;
			}
			else if(role == 'C'){
				state = S_STEADY_STATE;
			}
		}		
	}
}

//
// This function is called whenever a radio message is received
// You must copy any data you need out of the packet - as 'msgdata' will be overwritten by the next message
//
void application_radio_rx_msg(unsigned short dst, unsigned short src, int len, unsigned char *msgdata)
{
	if(role == 'C' && state == S_WAIT_FOR_JOIN_REQUESTS)
	{
		jr.type = msgdata[0];
		jr.src_id = msgdata[1];
		jr.data = msgdata[3];

		if(jr.type == P_JOINREQUEST){
			//save ids of cluster members
			cluster_members[num_of_cluster_members] = jr.src_id;
			num_of_cluster_members++;

			//if enough received then schedule TDMA and send to cluster members
			if(hw_timer_now_us() - time_holder > 5000000){ //TODO: this might be too long
				//schedule TDMA, send to cluster members
				//this is v. simple, just giving them an id and nodes wait based on that
				int i;
				for(i=0;i<num_of_cluster_members;i++){
					ts.type = P_TDMASCHEDULE;
					ts.src_id = node_id;
					ts.data = 0xFFFF;
					ts.tdma_slot = i;

					memcpy(&tx_buffer, &ts, sizeof(TDMASchedule));
					radio_send(tx_buffer, sizeof(TDMASchedule), cluster_members[i]);
				}

				state = S_STEADY_STATE;
			}
		}	
	}
	else if(role == 'N' && state == S_WAIT_FOR_ADVERTISEMENTS){
		
		
		//		if k annoucements have been found or enough time has passed, send Join-Request	
		ad.type = msgdata[0];
		ad.src_id = msgdata[1];
		ad.data = msgdata[3];

		//		check if clusterhead announcement
		if(ad.type == P_ADVERTISEMENT){
			//		get RSSI of annoucement
			unsigned char rssi = radio_rssi();
			if(rssi > max_rssi){
				max_rssi = rssi;
				id_of_max_rssi = ad.src_id;
				rssi_checked++;
			}

			if(rssi_checked == k || hw_timer_now_us() - time_holder > 5000000) //checked all cluster heads or 5 seconds has passed
			{	
				//reset vars
				rssi_checked = 0;
				max_rssi = 0;
				id_of_max_rssi = 0;

				//send a join-request to the best choice
				jr.data = 0xffff;
				jr.src_id = node_id;
				jr.type = P_JOINREQUEST;

				//Basic CSMA:
				int r;
				while(!CCA_STATUS){
					//as long as clear channel assessment says the channel is busy
					hw_timer_wait_milli(rand()%100); //TODO: this might be too long
				}

				memcpy(&tx_buffer, &jr, sizeof(JoinRequest));
				radio_send(tx_buffer, sizeof(JoinRequest), id_of_max_rssi);

				state = S_WAIT_FOR_SCHEDULE; 
			}
		}
	}
	else if(role == 'N' && state == S_WAIT_FOR_SCHEDULE){
		ts.type = msgdata[0];
		ts.src_id = msgdata[1];
		ts.data = msgdata[3];
		ts.tdma_slot = msgdata[5];

		if(ts.type == P_TDMASCHEDULE){
			hw_timer_wait_milli(100*ts.tdma_slot); //TODO: this is probably way too long
			
			// TODO: right here nodes send data AND their location data, energy level in one packet to the gateway. 
			//send data packet
			dp.data = 0xffff;
			dp.src_id = node_id;
			dp.type = P_DATAPACKET;

			memcpy(&tx_buffer, &dp, sizeof(DataPacket));
			radio_send(tx_buffer, sizeof(DataPacket), ts.src_id);

			state = S_WAIT_FOR_FORMATION;
		}	
	}
	else if(role == 'C' && state == S_STEADY_STATE){
		//		check if data
		//		record all data
		//		if data received from all members
		//			aggregate, send to gateway
		dp.type = msgdata[0];
		dp.src_id = msgdata[1];
		dp.data = msgdata[3];
		
		if(dp.type == P_DATAPACKET){
			data[data_received_count] = dp.data;
			data_received_count++;

			if(data_received_count == num_of_cluster_members){
				//in reality we aggregate, here data is just dummy


				// need to do some quick maths here - 
				//	- is it better to have no aggregation, and the data and node data gets sent together to the gateway?
				//	- it would depend on how much data there is I guess.
				//	-	maximum payload depends on upper layers, but at max max its 127 bytes.
				//	-   TODO: check how much is avaiable in RIME. 
				//	-	how much data is required about each node? Just 3 bytes?
				dp.type = P_DATAPACKET;
				dp.src_id = node_id;
				dp.data = 0xffff;

				memcpy(&tx_buffer, &dp, sizeof(DataPacket));
				radio_send(tx_buffer, sizeof(DataPacket), gateway_id);

				
				state = S_WAIT_FOR_FORMATION;
			}
		}	
	}
	else if(role == 'C' && state == S_WAIT_FOR_FORMATION){
		//read in formation data, send on to node(s?)
		fp.type = msgdata[0];
		fp.src_id = msgdata[1];
		fp.nodes = msgdata[3];
		fp.data = msgdata[5];
		fp.moreToCome = msgdata[7];
		//assuming just one for now.

		//read data about self 

		//forward to all nodes in cluster
		int i;
		for(i=0; i<num_of_cluster_members;i++){
			//TODO: TDMA??
			memcpy(&tx_buffer, &fp, sizeof(FormationPacket));
			radio_send(tx_buffer, sizeof(FormationPacket), cluster_members[i]);
		}

		//reset vars
		data_received_count = 0;
		num_of_cluster_members = 0;
		
		//TODO: set self to what was given in the fp packet 
		role = 'N';
		state = S_START;
	}
	else if(role == 'N' && state == S_WAIT_FOR_FORMATION){
		fp.type = msgdata[0];
		fp.src_id = msgdata[1];
		fp.nodes = msgdata[3];
		fp.data = msgdata[5];
		fp.moreToCome = msgdata[7];

		//TODO: set self to what was given in the fp packet
		role = 'N';
		state = S_START;
	}

	//acks will have to be included in this as well. 
	printf("Rx %d bytes: [%04x][%04x]\n", len, dst, src);
}

void application_radio_tx_done()
{
	tx_buffer_inuse = false;
}

void application_button_pressed(){}

void application_button_released(){}
