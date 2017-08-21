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
			// In the first message, the node includes location and energy level information, and fills up the rest with data
			// Beyond that, just data is sent until bytesOfData amount of data has been transmitted 
			if(role == 'N'){
				if(firstSend){
					hw_timer_wait_milli(100*tdma_slot);
					dpn.type = P_DATAPACKETWITHNODEINFO;
					dpn.data = 0xffff;
					dpn.src_id = node_id;
					dpn.locX = locX;
					dpn.locY = locY;
					dpn.energy = energy;

					memcpy(&tx_buffer, &dpn, sizeof(DataPacketWithNodeInfo));
					radio_send(tx_buffer, sizeof(DataPacketWithNodeInfo), ch_id);

					//TODO: acks?

					dataFilled += 118; //assuming 127 bytes per packet, minus the space taken up by the above.

					if(dataFilled > bytesOfData){
						state = S_WAIT_FOR_FORMATION;	
					}
					else{
						while(dataFilled < bytesOfData){
							hw_timer_wait_milli(100*tdma_slot);

							dp.type = P_DATAPACKET;
							dp.src_id = node_id;
							dp.data = 0xffff;
							
							memcpy(&tx_buffer, &dp, sizeof(DataPacket));
							radio_send(tx_buffer, sizeof(DataPacket), ch_id);

							dataFilled += 122;							
						}
					}
				}
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
			if(firstSend){
					hw_timer_wait_milli(100*tdma_slot);
					dpn.type = P_DATAPACKETWITHNODEINFO;
					dpn.data = 0xffff;
					dpn.src_id = node_id;
					dpn.locX = locX;
					dpn.locY = locY;
					dpn.energy = energy;

					memcpy(&tx_buffer, &dpn, sizeof(DataPacketWithNodeInfo));
					radio_send(tx_buffer, sizeof(DataPacketWithNodeInfo), ch_id);

					//TODO: acks?

					dataFilled += 118; //assuming 127 bytes per packet, minus the space taken up by the above.

					if(dataFilled > bytesOfData){
						state = S_WAIT_FOR_FORMATION;	
					}
					else{
						while(dataFilled < bytesOfData){
							hw_timer_wait_milli(100*tdma_slot);

							dp.type = P_DATAPACKET;
							dp.src_id = node_id;
							dp.data = 0xffff;
							
							memcpy(&tx_buffer, &dp, sizeof(DataPacket));
							radio_send(tx_buffer, sizeof(DataPacket), ch_id);

							dataFilled += 122;							
						}
						dataFilled = 0;
					}
				}
		}	
	}
	else if(role == 'C' && state == S_STEADY_STATE){
		//		check if data
		//		record all data
		//		if data received from all members
		//			aggregate, send to gateway
		dp.type = msgdata[0];;
		
		if(dp.type == P_DATAPACKET){
			dp.src_id = msgdata[1];
			dp.data = msgdata[3];
			dp.finished = msgdata[5];

			data[dp.src_id] = dp.data;
			
			if(dp.finished){
				data_received_count++;	
			}
			
		} else if(dp.type == P_DATAPACKETWITHNODEINFO){
			dpn.src_id = msgdata[1];
			dpn.energy = msgdata[3];
			dpn.locX = msgdata[5];
			dpn.locY = msgdata[7];
			dpn.data = msgdata[9];
			dpn.finished = msgdata[11];

			data[dp.src_id] = dp.data; // its just dummy data

			ep.type = P_ENERGYPACKET;
			ep.node_ids[ep.numNodes] = dpn.src_id;
			ep.energies[ep.numNodes] = dpn.energy;
			ep.locXs[ep.numNodes] = dpn.locX;
			ep.locYs[ep.numNodes] = dpn.locY;
			ep.numNodes++;

			if(dp.finished){
				data_received_count++;	
			}
		}	

		if(data_received_count == num_of_cluster_members){
			//in reality we aggregate, here data is just dummy

			//send on node data first, then data.
			//6 bytes per node in the cluster

			memcpy(&tx_buffer, &ep, sizeof(EnergyPacket));
			radio_send(tx_buffer, sizeof(EnergyPacket), gateway_id);
				
			state = S_WAIT_FOR_ENERGY_ACK;
		}

	}
	else if(role == 'C' && state == S_WAIT_FOR_ENERGY_ACK){
		ea.type = msgdata[0];
		ea.tdma_slot = msgdata[1];

		if(ea.type == P_ENERGYACK){
			//send data
			while(dataFilled < bytesOfData){
				hw_timer_wait_milli(100*ea.tdma_slot);

				dp.type = P_DATAPACKET;
				dp.src_id = node_id;
				dp.data = 0xffff;
							
				memcpy(&tx_buffer, &dp, sizeof(DataPacket));
				radio_send(tx_buffer, sizeof(DataPacket), gateway_id);
				//TODO: acks?
				dataFilled += 122;							
			}
			dataFilled = 0;	
			state = S_WAIT_FOR_FORMATION;
		}
	}
	else if(role == 'C' && state == S_WAIT_FOR_FORMATION){
		//read in formation data, send on to node(s?)
		fp.type = msgdata[0];
		fp.numNodes = msgdata[1];
		fp.assignedCHs = msgdata[3];

		//forward to all nodes in cluster
		int i;
		for(i=0; i<num_of_cluster_members;i++){
			memcpy(&tx_buffer, &fp, sizeof(FormationPacket));
			radio_send(tx_buffer, sizeof(FormationPacket), cluster_members[i]);
		}

		//read data about self 
		if(fp.assignedCHS[node_id] == node_id){
			role = 'C';
			//TODO: grab all cluster members
		}
		else {
			role = 'N';
			clusterhead = fp.assignedCHS[node_id];
		}

		//TODO: reset all vars
		data_received_count = 0;
		num_of_cluster_members = 0;
		
		state = S_START;
	}
	else if(role == 'N' && state == S_WAIT_FOR_FORMATION){
		fp.type = msgdata[0];
		fp.numNodes = msgdata[1];
		fp.assignedCHs = msgdata[3];

		//read data about self 
		if(fp.assignedCHS[node_id] == node_id){
			role = 'C';
			//TODO: grab all cluster members
		}
		else {
			role = 'N';
			clusterhead = fp.assignedCHS[node_id];
		}

		//TODO: reset all vars
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
