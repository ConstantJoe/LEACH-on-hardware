#include "watchdog.h"
#include "net/rime.h"
#include "message_structs.h"


#define BUFFER_LEN                  128
static unsigned char msgdata[BUFFER_LEN];

DataPacket dp;

PROCESS(gateway_process, "Gateway process");

AUTOSTART_PROCESSES(&gateway_process);

static struct unicast_conn uc;

static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
	//in reality data will be saved to somewhere, but this is fine for now.
	packetbuf_copyto(&msgdata);

	dp.type = msgdata[0];    
    dp.src_id.u8[0] = msgdata[1];
    dp.src_id.u8[1] = msgdata[2];  
    dp.data = msgdata[3]; 

    printf("unicast message received from %d.%d\n", from->u8[0], from->u8[1]);
}

static void
sent_uc(struct unicast_conn *c, int status, int num_tx)
{
	const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  	if(linkaddr_cmp(dest, &linkaddr_null)) {
    	return;
  	}
  	
  	printf("unicast message sent to %d.%d: status %d num_tx %d\n", dest->u8[0], dest->u8[1], status, num_tx);
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};


PROCESS_THREAD(gateway_process, ev, data)
{
	PROCESS_EXITHANDLER(unicast_close(&uc);)

	PROCESS_BEGIN();

	unicast_open(&uc, 146, &unicast_callbacks);

	printf("Startup complete.\r\n");

	PROCESS_END();
}