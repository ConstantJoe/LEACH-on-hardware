
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

typedef EnergyPacket
{
	uint8_t type;
	uint16_t numNodes;
	uint16_t node_ids;
	uint16_t energies[30];	//TODO: figure this later
	uint16_t locXs[30];
	uint16_t locYs[30];
} EnergyPacket;

typedef EnergyAck 
{
	uint8_t type;
	uint16_t tdma_slot;
} EnergyAck;

typedef FormationPacket
{
	uint8_t type;
	uint16_t numNodes;
	uint16_t assignedCHs[200];
} FormationPacket;