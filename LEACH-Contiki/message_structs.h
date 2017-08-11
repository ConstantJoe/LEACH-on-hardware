
typedef struct Advertisement {
	uint8_t type;
	linkaddr_t src_id;
	uint16_t data;
} Advertisement;

typedef struct JoinRequest {
	uint8_t type;
	linkaddr_t src_id;
	uint16_t data;
} JoinRequest;

typedef struct TDMASchedule {
	uint8_t type;
	linkaddr_t src_id;
	uint16_t data;
	uint16_t tdma_slot; 
} TDMASchedule;

typedef struct DataPacket
{
	uint8_t type;
	linkaddr_t src_id;
	uint16_t data;
} DataPacket;