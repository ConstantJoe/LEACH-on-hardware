//
// Atmega128RFA1 radio code
//

#include <avr/io.h>

#include "radio.h"
#include "hw_timer.h"

#define PAN_ID_LOW  0x00
#define PAN_ID_HIGH 0x00

static unsigned char radio_state=0;
static char radio_maxRSSI=-1;
static unsigned short radio_macaddr=0x0000;

static unsigned char radio_energy=0;
static unsigned char radio_lqi=0;

void radio_init(unsigned short macaddr)
{
   // reset
   TRXPR = 0;
   hw_timer_wait_micro(6);
   TRXPR = (1<<TRXRST);

   // get the state
   radio_state = TRX_STATUS;

   // after hardware reset, transfer to TRX_OFF state
   if (radio_state==P_ON) {
      TRX_STATE = CMD_TRX_OFF;
      hw_timer_wait_micro(510);
      radio_state = TRX_STATUS;
   }

   // set the channel
   PHY_CC_CCA = F_2405MHZ;

   // clear the IRQ_STATUS register
   IRQ_STATUS = 0xFF;

   // set & store our mac addr
   radio_macaddr = macaddr;
   SHORT_ADDR_0 = (unsigned char)(macaddr & 0xFF); // low byte
   SHORT_ADDR_1 = (unsigned char)(macaddr >> 8);   // high byte

   // set the PAN ID
   PAN_ID_0 = PAN_ID_LOW;
   PAN_ID_1 = PAN_ID_HIGH;

   // get the state
   radio_state = TRX_STATUS;
}

void radio_start()
{
   // clear the IRQ_STATUS register
   IRQ_STATUS = 0xFF;

   // enter rx state
   TRX_STATE = CMD_RX_ON;
   while ((TRX_STATUS&0x1F)!=RX_ON);

   // set RX_SAFE_MODE (prevent received packets from overwriting the buffer)
   TRX_CTRL_2 |= 0x80;
}


static unsigned char radio_txdone_flag=0;

unsigned char radio_tx_done()
{
	unsigned char temp=radio_txdone_flag;
	radio_txdone_flag=0;
	return temp;
}

void radio_send(unsigned char *buffer, unsigned char len, unsigned short dst)
{
   unsigned char i;
   unsigned char *bp;
   bp=(unsigned char *)0x180;

   // clear the IRQ_STATUS register
   IRQ_STATUS = 0xFF;

   // enter PLL_ON state
   if ((TRX_STATUS&0x1F) != PLL_ON) {
      TRX_STATE = CMD_PLL_ON;
      while (TRX_STATUS != PLL_ON);//XXX
   }

   // set the length - this is the PHY length, so add 2 for the MAC layer FCS!
   *bp++ = len+13; // header and crc

   // write the 802.14.5 header

   *bp++ = 0x20; // FCF: Frame Type=data
   *bp++ = 0x22; //      Addressing mode=16-bit short address

   *bp++ = 0x01; // Sequence number (not used)

   *bp++ = PAN_ID_HIGH; // Destination PAN
   *bp++ = PAN_ID_LOW;

   *bp++ = (unsigned char)(dst>>8); // destination address: high-order byte
   *bp++ = (unsigned char)(dst&0xFF); //                    low-order byte

   *bp++ = PAN_ID_HIGH; // Source PAN
   *bp++ = PAN_ID_LOW;

   *bp++ = (unsigned char)(radio_macaddr>>8);   // source address: high-order byte
   *bp++ = (unsigned char)(radio_macaddr&0xFF); //                 low-order byte

   // copy the data
   for (i=0; i<len; i++)
      *bp++ = *buffer++;

   // write two zeros that will be overwritten by the FCS
   *bp++ = 0x00;
   *bp++ = 0x00;

   // start the transmit
   TRX_STATE = CMD_TX_START;

   // wait for the transmit to end
   while ((IRQ_STATUS&(1<<TX_END))==0);

   // clear the IRQ_STATUS register
   IRQ_STATUS = 0xFF;

   // enter rx state
   TRX_STATE = CMD_RX_ON;
   while ((TRX_STATUS&0x1F)!=RX_ON);

   // set the tx done flag
   radio_txdone_flag = 1;

   // record final state
   radio_state = TRX_STATUS;
}

unsigned char radio_rxed()
{
	if ((IRQ_STATUS&(1<<RX_END))==0)
	   return 0;
	else
	   return 1;
}

unsigned char radio_recv(unsigned char *buffer, unsigned char buffer_len,
   unsigned short *dst, unsigned short *src,
   unsigned char *valid_crc, unsigned char *energy, unsigned char *lqi)
{
   unsigned char i;
   unsigned char *bp;
   unsigned char rxlen=*(unsigned char *)0x17B;
   bp=(unsigned char *)0x180;

   // reset the max rssi indicator
   radio_maxRSSI = -1;

   // wait for rx start
   while ((IRQ_STATUS&(1<<RX_START))==0);

   // wait for rx end
   while ((IRQ_STATUS&(1<<RX_END))==0) {
      if ((PHY_RSSI & 0x1F) > radio_maxRSSI)
         radio_maxRSSI = (PHY_RSSI & 0x1F);
   }

   // get the received length (for now, just truncate packets larger than the buffer without warning)
   rxlen = rxlen - 11; // calculate the payload length
   if (rxlen >= buffer_len)
      rxlen = buffer_len;

   // get the addresses

   *dst = ((*(unsigned char *)(bp+5))<<8) | (*(unsigned char *)(bp+6)); // high,low
   *src = ((*(unsigned char *)(bp+9))<<8) | (*(unsigned char *)(bp+10)); // high,low

   // skip the header

   bp += 11;

   // copy the data
   for (i=0; i<rxlen; i++)
      buffer[i] = *bp++;

   // get lqi & ed
   radio_lqi = *bp++;
   *lqi = radio_lqi;
   radio_energy = PHY_ED_LEVEL;
   *energy = radio_energy;
   *valid_crc = (PHY_RSSI>>7);

   // wait until we return to the RX_ON state
   while ((TRX_STATUS&0x1F) != RX_ON);

   // clear and then set RX_SAFE_MODE here
   TRX_CTRL_2 &= 0x7F; // clear RX_SAFE_MODE (enable next rx)
   TRX_CTRL_2 |= 0x80; // set RX_SAFE_MODE (prevent subsequent receives)

   // clear the IRQ_STATUS register
   IRQ_STATUS = 0xFF;

   // record final state
   radio_state = TRX_STATUS;

   return rxlen;
}

//
// level
//     0   3.0 dBm 
//     1   2.8 dBm 
//     2   2.3 dBm 
//     3   1.8 dBm 
//     4   1.3 dBm 
//     5   0.7 dBm
//     6   0.0 dBm 
//     7  -1   dBm 
//     8  -2   dBm 
//     9  -3   dBm 
//    10  -4   dBm 
//    11  -5   dBm 
//    12  -7   dBm 
//    13  -9   dBm 
//    14 -12   dBm 
//    15 -17   dBm 
//
void radio_set_power(unsigned char level) {
   unsigned char temp=PHY_TX_PWR;
   PHY_TX_PWR = (temp & 0xF0) | (level & 0x0F);
}

unsigned char radio_rssi()
{
   unsigned char temp=(PHY_RSSI & 0x1F);
   if (temp>radio_maxRSSI)
      radio_maxRSSI = temp;
   return temp;
}

char radio_max_rssi()
{
   char temp=radio_maxRSSI;
   radio_maxRSSI = 0;
   return temp;
}


unsigned char radio_latest_energy() {
   return radio_energy;
}

unsigned char radio_latest_lqi() {
   return radio_lqi;
}

void radio_shutdown() 
{
  // reset
  TRXPR = 0;
  hw_timer_wait_micro(6);
  TRXPR = (1<<TRXRST);

  // go to sleep
  TRXPR |= (1<<SLPTR);
}