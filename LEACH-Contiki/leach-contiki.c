/*
  Written by J. Finnegan.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "watchdog.h"
#include "dev/leds.h"
#include "dev/rs232.h"
#include "serial-line-dual.h"
#include "dev/button-sensor.h"
//#include "radio/rf230bb/rf230bb.c"
#include "net/rime/rime.h"
//#include "net/rime/broadcast.h"


#define S_START                     0
#define S_WAIT_FOR_JOIN_REQUESTS    1
#define S_STEADY_STATE              2
#define S_WAIT_FOR_ADVERTISEMENTS   3
#define S_WAIT_FOR_SCHEDULE         4

int state = S_START;

/*---------------------------------------------------------------------------*/
//PROCESS(relay_process, "Relay process");
//PROCESS(button_process, "Button process");
PROCESS(timer_process, "Timer process");

AUTOSTART_PROCESSES(&timer_process);

/*from example-broadcast.c*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  rs232_print(RS232_PORT_0, "Message received.\r\n");
  rs232_print(RS232_PORT_0, (char *)packetbuf_dataptr());

  if(role == 'N' && state == S_WAIT_FOR_ADVERTISEMENTS){
      //    if k annoucements have been found or enough time has passed, send Join-Request  
    ad.type = msgdata[0];    //TODO: needs to be changed
    ad.src_id = msgdata[1];   //TODO: needs to be changed
    ad.data = msgdata[3];      //TODO: needs to be changed

    //    check if clusterhead announcement
    if(ad.type == P_ADVERTISEMENT){
      //    get RSSI of annoucement
      unsigned char rssi = radio_rssi();     //TODO: needs to be changed
      if(rssi > max_rssi){
        max_rssi = rssi;
        id_of_max_rssi = ad.src_id;
        rssi_checked++;
      }

       //TODO: needs to be changed
      if(rssi_checked == k || timer_now_us() - time_holder > 5000000) //checked all cluster heads or 5 seconds has passed
      { 
        //reset vars
        rssi_checked = 0;
        max_rssi = 0;
        id_of_max_rssi = 0;

        //send a join-request to the best choice
        jr.data = 0xffff;      //TODO: needs to be changed
        jr.src_id = node_id;     //TODO: needs to be changed
        jr.type = P_JOINREQUEST;   //TODO: needs to be changed

        //Basic CSMA:
        int r;
        while(!CCA_STATUS){    //TODO: needs to be changed
          //as long as clear channel assessment says the channel is busy
          timer_wait_milli(rand()%100); //TODO: this might be too long  //TODO: needs to be changed
        }

        memcpy(&tx_buffer, &jr, sizeof(JoinRequest));  //TODO: needs to be changed
        radio_send(tx_buffer, sizeof(JoinRequest), id_of_max_rssi);  //TODO: needs to be changed

        state = S_WAIT_FOR_SCHEDULE; 
      }
    }

  }
  //printf("broadcast message received from %d.%d: '%s'\n",
  //       from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast message received from %d.%d\n",
   from->u8[0], from->u8[1]);

  if(role == 'C' && state == S_WAIT_FOR_JOIN_REQUESTS)
  {
    jr.type = msgdata[0];         //TODO: needs to be changed
    jr.src_id = msgdata[1];       //TODO: needs to be changed
    jr.data = msgdata[3];         //TODO: needs to be changed

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
          ts.type = P_TDMASCHEDULE;     //TODO: needs to be changed
          ts.src_id = node_id;          //TODO: needs to be changed
          ts.data = 0xFFFF;             //TODO: needs to be changed
          ts.tdma_slot = i;             //TODO: needs to be changed

          memcpy(&tx_buffer, &ts, sizeof(TDMASchedule));                    //TODO: needs to be changed
          radio_send(tx_buffer, sizeof(TDMASchedule), cluster_members[i]);  //TODO: needs to be changed
        }

        state = S_STEADY_STATE;
      }
    } 
  }
  else if(role == 'N' && state == S_WAIT_FOR_SCHEDULE){
    ts.type = msgdata[0];          //TODO: needs to be changed
    ts.src_id = msgdata[1];         //TODO: needs to be changed
    ts.data = msgdata[3];            //TODO: needs to be changed
    ts.tdma_slot = msgdata[5];         //TODO: needs to be changed

    if(ts.type == P_TDMASCHEDULE){
      timer_wait_milli(100*ts.tdma_slot); //TODO: this is probably way too long
       //TODO: needs to be changed
      //send data packet
      dp.data = 0xffff;
      dp.src_id = node_id;
      dp.type = P_DATAPACKET;

      memcpy(&tx_buffer, &dp, sizeof(DataPacket));
      radio_send(tx_buffer, sizeof(DataPacket), ts.src_id);

      state = S_START;
    } 
  }
  else if(role == 'C' && state == S_STEADY_STATE){
    //    check if data
    //    record all data
    //    if data received from all members
    //      aggregate, send to gateway
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
        role = 'N';
        state = S_START;
      }
    } 
  }
}
/*---------------------------------------------------------------------------*/
static void
sent_uc(struct unicast_conn *c, int status, int num_tx)
{
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: status %d num_tx %d\n",
    dest->u8[0], dest->u8[1], status, num_tx);
}
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/



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


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(timer_process, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);
  unicast_open(&uc, 146, &unicast_callbacks);

  /* Delay 10 seconds */
  etimer_set(&et, CLOCK_SECOND*10);

  while(1) {

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

      //update round info
    if(state == S_START){
      round_no++;
      rounds_since_ch++;

      //decide whether this is a cluster head or not
      k = ClusterOptimum();

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
          ad.data = 0xffff;     //TODO: needs to be changed
          ad.src_id = node_id;  //TODO: needs to be changed  
          ad.type = P_ADVERTISEMENT; //TODO: needs to be changed

          //Basic CSMA:
          int r;
          while(!CCA_STATUS){ //TODO: needs to be changed
              //as long as clear channel assessment says the channel is busy
              timer_wait_milli(rand()%100); //TODO: this might be too long //TODO: needs to be changed
            }

          memcpy(&tx_buffer, &ad, sizeof(Advertisement)); //TODO: needs to be changed
          radio_send(tx_buffer, sizeof(Advertisement), 0xFF); //broadcast //TODO: needs to be changed

          state = S_WAIT_FOR_JOIN_REQUESTS;
          rounds_since_ch = 0;

          time_holder = timer_now_us(); //TODO: needs to be changed
        }
        else if(role == 'N'){
          state = S_WAIT_FOR_ADVERTISEMENTS;
          time_holder = timer_now_us(); //TODO: needs to be changed
        }  

        etimer_reset(&et);
      }
    }
    PROCESS_END();
  }
/*---------------------------------------------------------------------------*/

