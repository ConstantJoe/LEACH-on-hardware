
typedef struct Advertisement {
	uint8_t type;
	uint16_t src_id;
	uint16_t data;			//TODO: this isn't needed?
} Advertisement;

typedef struct JoinRequest {
	uint8_t type;
	uint16_t src_id;
	uint16_t data;			//TODO: this isn't needed?
} JoinRequest;

typedef struct TDMASchedule {
	uint8_t type;
	uint16_t src_id;
	uint16_t data;			//TODO: this isn't needed?
	uint16_t tdma_slot; 
} TDMASchedule;

typedef struct DataPacket
{
	uint8_t type;
	uint16_t src_id;
	uint16_t data;			//TODO: expand this to be something realistic
} DataPacket;

typedef struct FormationPacket
{
	uint8_t type;
	uint16_t src_id;
	uint16_t nodes;
	uint16_t data;			//TODO: expand this to fill 3*nodes bytes.
	uint8_t moreToCome;
}