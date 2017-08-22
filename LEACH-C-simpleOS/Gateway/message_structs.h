
typedef struct Advertisement {
	uint8_t type;
	uint16_t src_id;
	uint16_t data;
} Advertisement;

typedef struct JoinRequest {
	uint8_t type;
	uint16_t src_id;
	uint16_t data;
} JoinRequest;

typedef struct TDMASchedule {
	uint8_t type;
	uint16_t src_id;
	uint16_t data;
	uint16_t tdma_slot; 
} TDMASchedule;

typedef struct DataPacketWithNodeInfo
{
	uint8_t type;
	uint16_t src_id;
	uint16_t energy;
	uint16_t locX;
	uint16_t locY;
	uint16_t data;
	uint16_t finished; 
} DataPacketWithNodeInfo;

typedef struct DataPacket
{
	uint8_t type;
	uint16_t src_id;
	uint16_t data;
	uint16_t finished;
} DataPacket;

typedef struct EnergyPacket
{
	uint8_t type;
	uint16_t num_nodes;
	uint16_t node_ids[15];
	uint16_t energies[15];	//128 is max bytes - do all these have to be 16bit?
	uint16_t loc_xs[15];
	uint16_t loc_ys[15];
} EnergyPacket;

typedef struct EnergyAck 
{
	uint8_t type;
	uint16_t tdma_slot;
} EnergyAck;

typedef struct FormationPacket
{
	uint8_t type;
	uint16_t numNodes;
	uint16_t assignedCHs[62]; //128 is max bytes - can IDs be 8 bit instead?
} FormationPacket;