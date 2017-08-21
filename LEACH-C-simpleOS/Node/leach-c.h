#define S_START						0
#define S_WAIT_FOR_JOIN_REQUESTS 	1
#define S_STEADY_STATE 				2
#define S_WAIT_FOR_ADVERTISEMENTS	3
#define S_WAIT_FOR_SCHEDULE			4
#define S_WAIT_FOR_FORMATION		5


#define P_ADVERTISEMENT				1
#define P_JOINREQUEST				2
#define P_TDMASCHEDULE				3
#define P_DATAPACKET				4
#define P_DATAPACKETWITHNODEINFO	5

#define NUMNODES 					100


static timer timer1;

int start = 1;

// Buffer for transmitting radio packets
unsigned char tx_buffer[RADIO_MAX_LEN];
bool tx_buffer_inuse=false; // Check false and set to true before sending a message. Set to false in tx_done

int state = S_START;
int node_id = 0x0002;
int gateway_id = 0x0001;

// given to nodes by the gateway
int ch_id = 0x0002;
int tdma_slot = 0;

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

Advertisement ad;
JoinRequest jr;
TDMASchedule ts;
DataPacket dp;
FormationPacket fp;
DataPacketWithNodeInfo dpn;
EnergyPacket ep;


int bytesOfData = 100;

int firstSend = 1;

uint16_t locX = 10; //just filler for now
uint16_t locY = 10;
uint16_t energy = 1000;

int dataFilled = 0;

double clusterOptimum(void);