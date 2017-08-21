/*
  Written by J. Finnegan.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "watchdog.h"
#include "dev/leds.h"
#include "dev/rs232.h"
//#include "serial-line-dual.h"
#include "dev/button-sensor.h"
#include "net/rime.h"
#include "message_structs.h"

//#include "radio/rf230bb/rf230bb.h" //for avr radio, swapped out for cooja one
#include "dev/cooja-radio.h" //for virtual cooja radio

#define S_START                     0
#define S_WAIT_FOR_JOIN_REQUESTS    1
#define S_STEADY_STATE              2
#define S_WAIT_FOR_ADVERTISEMENTS   3
#define S_WAIT_FOR_SCHEDULE         4

#define P_ADVERTISEMENT             1
#define P_JOINREQUEST               2
#define P_TDMASCHEDULE              3
#define P_DATAPACKET                4

#define NUMNODES                    100


#define BUFFER_LEN                  128
static unsigned char msgdata[BUFFER_LEN];


/*
 *  TODO: 
 *     see if DSSS can be used
 *     cluster optimum - use version in the paper
 *     ack packets 
 *     make use of sent_uc when needed
 *     fix timings
 *     modify Makefile to include message_structs.h
 *     general cleanup and comments
 */ 

int state = S_START;
int k;

unsigned long time_holder = 0;

int rssi_checked = 0;

//uint8_t max_rssi = 0;
int max_rssi = 0;
linkaddr_t id_of_max_rssi;
char role = 'N';

linkaddr_t cluster_members[NUMNODES];
int num_of_cluster_members = 0; 

uint16_t data[NUMNODES];
int data_received_count = 0;

int num_dead = 0;
int area_height = 100;
int area_width = 100;
int sink_x = 50;
int sink_y = 50;
double e_freespace = 10*0.000000000001;
double e_multipath = 0.0013*0.000000000001;

int round_no = 0;
int rounds_since_ch = 0;


linkaddr_t node_addr; 
linkaddr_t gateway_addr; 

Advertisement ad;
JoinRequest jr;
TDMASchedule ts;
DataPacket dp;

/*---------------------------------------------------------------------------*/
PROCESS(timer_process, "Timer process");

AUTOSTART_PROCESSES(&timer_process);

static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  packetbuf_copyto(&msgdata);
  //rs232_print(RS232_PORT_0, "Message received.\r\n");         //for rfa1
  //rs232_print(RS232_PORT_0, (char *)packetbuf_dataptr());
  //printf("Message received.\r\n");
  //printf("from %d, %d", from->u8[0], from->u8[1]);

  if(role == 'N' && state == S_WAIT_FOR_ADVERTISEMENTS){
    //    if k annoucements have been found or enough time has passed, send Join-Request  
    ad.type = msgdata[0];    
    ad.src_id.u8[0] = msgdata[1];
    ad.src_id.u8[1] = msgdata[2];   
    ad.data = msgdata[3]; 

    //    check if clusterhead announcement
    if(ad.type == P_ADVERTISEMENT){
      //    get RSSI of annoucement
      //uint8_t rssi = rf230_get_raw_rssi(); //avr version, is this new enough?
      int  rssi = radio_signal_strength_last(); //cooja version
      //int rss_val = cc2420_last_rssi; //cooja version
      //int rss_offset = -45;
      //int rssi = rss_val + rss_offset;
      
      if(rssi > max_rssi){

        max_rssi = rssi;
        id_of_max_rssi.u8[0] = from->u8[0];
        id_of_max_rssi.u8[1] = from->u8[1];
        rssi_checked++;
      }

      if(rssi_checked == k || clock_seconds() - time_holder > 5) //checked all cluster heads or 5 seconds has passed
      { 
        //send a join-request to the best choice
        jr.data = 0xffff;      
        jr.src_id = node_addr;  
        jr.type = P_JOINREQUEST;  

        // According to here: https://sourceforge.net/p/contiki/mailman/message/22285607/
        // Atmel radios in Contiki do a CCA check by default - so all transmissions are CSMA-like?

        packetbuf_copyfrom(&jr, sizeof(JoinRequest));
        if(!linkaddr_cmp(&id_of_max_rssi, &linkaddr_node_addr)) {
            unicast_send(&uc, &id_of_max_rssi);
        }
        
        //reset vars
        rssi_checked = 0;
        max_rssi = 0;
        id_of_max_rssi.u8[0] = 0;
        id_of_max_rssi.u8[1] = 0;

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
  packetbuf_copyto(&msgdata);
  //printf("unicast message received from %d.%d\n",
  // from->u8[0], from->u8[1]);

  if(role == 'C' && state == S_WAIT_FOR_JOIN_REQUESTS)
  {
    jr.type = msgdata[0];     
    jr.src_id.u8[0] = msgdata[1];
    jr.src_id.u8[1] = msgdata[2];           
    jr.data = msgdata[3];      

    if(jr.type == P_JOINREQUEST){ 
      //save ids of cluster members
      cluster_members[num_of_cluster_members] = jr.src_id;    
      num_of_cluster_members++;

      //if enough received then schedule TDMA and send to cluster members
      if(clock_seconds() - time_holder > 5){ //TODO: this might be too long
        //schedule TDMA, send to cluster members
        //this is v. simple, just giving them an id and nodes wait based on that
        int i;
        for(i=0;i<num_of_cluster_members;i++){
          ts.type = P_TDMASCHEDULE;     
          ts.src_id = node_addr;   
          ts.data = 0xFFFF;             
          ts.tdma_slot = i;             

          packetbuf_copyfrom(&ts, sizeof(TDMASchedule));
          if(!linkaddr_cmp(&cluster_members[i], &linkaddr_node_addr)) {
              unicast_send(&uc, &cluster_members[i]);
          }
        }

        state = S_STEADY_STATE;
      }
    } 
  }
  else if(role == 'N' && state == S_WAIT_FOR_SCHEDULE){
    ts.type = msgdata[0];          
    ts.src_id.u8[0] = msgdata[1];
    ts.src_id.u8[1] = msgdata[2];
    ts.data = msgdata[3];           
    ts.tdma_slot = msgdata[5];    

    if(ts.type == P_TDMASCHEDULE){
      clock_wait(100*ts.tdma_slot); //TODO:  the input is in ticks, need to find out how long a tick is.

      //send data packet
      dp.data = 0xffff;      
      dp.src_id = node_addr;  
      dp.type = P_DATAPACKET;

      //linkaddr_t src_addr; = {msgdata[1], msgdata[2]};
      //id_of_max_rssi.u8[0] = from->u8[0];
      //id_of_max_rssi.u8[1] = from->u8[1];

      packetbuf_copyfrom(&dp, sizeof(DataPacket));
      if(!linkaddr_cmp(from, &linkaddr_node_addr)) {
          unicast_send(&uc, from);
      }

      state = S_START;
    } 
  }
  else if(role == 'C' && state == S_STEADY_STATE){
    //    check if data
    //    record all data
    //    if data received from all members
    //      aggregate, send to gateway
    dp.type = msgdata[0];    
    dp.src_id.u8[0] = msgdata[1];
    dp.src_id.u8[1] = msgdata[2];  
    dp.data = msgdata[3];  
    
    if(dp.type == P_DATAPACKET){
      data[data_received_count] = dp.data;
      data_received_count++;

      if(data_received_count == num_of_cluster_members){
        //in reality we aggregate, here data is just dummy
        dp.type = P_DATAPACKET;   
        dp.src_id = node_addr;     
        dp.data = 0xffff;       

        packetbuf_copyfrom(&dp, sizeof(DataPacket));
        if(!linkaddr_cmp(&gateway_addr, &linkaddr_node_addr)) {
          unicast_send(&uc, &gateway_addr);
        }

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
  return;

  //TODO: have a think about this - it might be better to move everything after the sends from above to here. 

  /*const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: status %d num_tx %d\n",
    dest->u8[0], dest->u8[1], status, num_tx);*/
}
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};




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

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  static struct etimer et;
  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);
  unicast_open(&uc, 146, &unicast_callbacks);

  /* Delay 10 seconds */
  etimer_set(&et, CLOCK_SECOND*10);


  node_addr.u8[0] = 0;
  node_addr.u8[1] = 1;

  gateway_addr.u8[0] = 0;
  gateway_addr.u8[1] = 1;

  printf("Startup complete.\r\n");

  while(1) {

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

      //update round info
    if(state == S_START){
      round_no++;
      rounds_since_ch++;

      //decide whether this is a cluster head or not
      k = clusterOptimum();

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
          role = 'C';
        }
        else{
          role = 'N';
        }
      }

      if(role == 'C'){
          // send ADV message   
          ad.data = 0xffff;           
          ad.src_id = node_addr;       
          ad.type = P_ADVERTISEMENT;  

          // According to here: https://sourceforge.net/p/contiki/mailman/message/22285607/
          // Atmel radios in Contiki do a CCA check by default - so all transmissions are CSMA-like?

          packetbuf_copyfrom(&ad, sizeof(Advertisement));
          broadcast_send(&broadcast);

          state = S_WAIT_FOR_JOIN_REQUESTS;
          rounds_since_ch = 0;

          time_holder = clock_seconds();
        }
        else if(role == 'N'){
          state = S_WAIT_FOR_ADVERTISEMENTS;
          time_holder = clock_seconds();
        }  

        etimer_reset(&et);
      }
    }
    PROCESS_END();
  }
/*---------------------------------------------------------------------------*/

