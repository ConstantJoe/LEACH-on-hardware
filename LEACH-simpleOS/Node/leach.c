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

//TODO: adjust power of node transceiver based on RSSI
//		RFA1 supports DSSS - find out how it works
//		memory management
//		cluster optimum
//		ack packets
//		general cleanup
//		also need to do the gateway

#define S_START						0
#define S_WAIT_FOR_JOIN_REQUESTS 	1
#define S_STEADY_STATE 				2
#define S_WAIT_FOR_ADVERTISEMENTS	3
#define S_WAIT_FOR_SCHEDULE			4


#define P_ADVERTISEMENT				1
#define P_JOINREQUEST				2
#define P_TDMASCHEDULE				3
#define P_DATAPACKET				4

#define NUMNODES 					100
// Buffer for transmitting radio packets
unsigned char tx_buffer[RADIO_MAX_LEN];
bool tx_buffer_inuse=false; // Check false and set to true before sending a message. Set to false in tx_done

int state = S_START;
int node_id = 0x0002;
int gateway_id = 0x0001;

int packet_pize = 6400;
int ctr_packet_size = 6400;
int round_no = 0;
int rounds_since_ch = 0;

int num_dead = 0;
int area_height = 100;
int area_width = 100;
int sink_x = 50;
int sink_y = 50;

uint16_t cluster_members[NUMNODES];
int num_of_cluster_members = 0; 

uint16_t data[NUMNODES];
int data_received_count = 0;

char role = 'N';

double e_freespace = 10*0.000000000001;
double e_multipath = 0.0013*0.000000000001;

int id_of_max_rssi = 0;
unsigned char max_rssi = 0;
int rssi_checked = 0;

unsigned long time_holder = 0;

double k; //optimum number of clusters

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
	timer_init(&timer1, TIMER_MILLISECONDS, 30000, 250); //TODO: time between ticks might change
}

//
// Timer tick handler
//
void application_timer_tick(timer *t)
{
	k = ClusterOptimum();
	//set C(i)
	// if a node has been a CH in the most recent r%(N/k) rounds, then C(i) = 0. Else C(i) = 1.
	int c_i;
	//decide if this node is a CH


	if(role == 'C'){
		// send ADV message
		Advertisement ad;
		ad.data = 0xffff;
		ad.src_id = node_id;
		ad.type = P_ADVERTISEMENT;

		//Basic CSMA:
		int r;
		while(!CCA_STATUS){
			//as long as clear channel assessment says the channel is busy
			timer_wait_milli(rand()%100); //TODO: this might be too long
		}

		memcpy(&tx_buffer, &ad, sizeof(Advertisement));
		radio_send(tx_buffer, sizeof(Advertisement), 0xFF); //broadcast

		state = S_WAIT_FOR_JOIN_REQUESTS;
		time_holder = timer_now_us(); 
	}
	else if(role == 'N'){
		state = S_WAIT_FOR_ADVERTISEMENTS;
		time_holder = timer_now_us();
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
		JoinRequest jr;
		jr.type = msgdata[0];
		jr.src_id = msgdata[1];
		jr.data = msgdata[3];

		if(jr.type == P_JOINREQUEST){
			//save ids of cluster members
			cluster_members[num_of_cluster_members] = jr.src_id;
			num_of_cluster_members++;

			//if enough received then schedule TDMA and send to cluster members
			if(timer_now_us() - time_holder > 5000000){ //TODO: this might be too long
				//schedule TDMA, send to cluster members
				//this is v. simple, just giving them an id and nodes wait based on that
				int i;
				for(i=0;i<num_of_cluster_members;i++){
					TDMASchedule ts;
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
		Advertisement ad;
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

			if(rssi_checked == k || timer_now_us() - time_holder > 5000000) //checked all cluster heads or 5 seconds has passed
			{	
				//reset vars
				rssi_checked = 0;
				max_rssi = 0;
				id_of_max_rssi = 0;

				//send a join-request to the best choice
				JoinRequest jr;
				jr.data = 0xffff;
				jr.src_id = node_id;
				jr.type = P_JOINREQUEST;

				//Basic CSMA:
				int r;
				while(!CCA_STATUS){
					//as long as clear channel assessment says the channel is busy
					timer_wait_milli(rand()%100); //TODO: this might be too long
				}

				memcpy(&tx_buffer, &jr, sizeof(JoinRequest));
				radio_send(tx_buffer, sizeof(JoinRequest), id_of_max_rssi);

				state = S_WAIT_FOR_SCHEDULE; 
			}
		}
	}
	else if(role == 'N' && state == S_WAIT_FOR_SCHEDULE){
		TDMASchedule ts;
		ts.type = msgdata[0];
		ts.src_id = msgdata[1];
		ts.data = msgdata[3];
		ts.tdma_slot = msgdata[5];

		if(ts.type == P_TDMASCHEDULE){
			timer_wait_milli(100*ts.tdma_slot); //TODO: this is probably way too long
			
			//send data packet
			DataPacket dp;
			dp.data = 0xffff;
			dp.src_id = node_id;
			dp.type = P_DATAPACKET;

			memcpy(&tx_buffer, &dp, sizeof(DataPacket));
			radio_send(tx_buffer, sizeof(DataPacket), ts.src_id);
		}	
	}
	else if(role == 'C' && state == S_STEADY_STATE){
		//		check if data
		//		record all data
		//		if data received from all members
		//			aggregate, send to gateway
		DataPacket dp;
		dp.type = msgdata[0];
		dp.src_id = msgdata[1];
		dp.data = msgdata[3];
		
		if(dp.type == P_DATAPACKET){
			data[data_received_count] = dp.data;
			data_received_count++;

			if(data_received_count == num_of_cluster_members){
				//in reality we aggregate, here data is just dummy

				dp.type = P_DATAPACKET;
				dp.src_id = node_id;
				dp.data = 0xffff;

				memcpy(&tx_buffer, &dp, sizeof(DataPacket));
				radio_send(tx_buffer, sizeof(DataPacket), gateway_id);

				data_received_count = 0;
				num_of_cluster_members = 0;
			}
		}	
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
