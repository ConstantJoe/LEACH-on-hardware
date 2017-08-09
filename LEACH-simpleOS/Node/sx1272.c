#include <stdio.h>
#include "Ctk_spi.h"
#include "sx1272.h"
#include <avr/io.h>

/*----------------------------------------------------------------------------------------------*/

void sx1272_init() // initialise spi and the sx1272 device
{
	spix_select(SX1272);
	spix_init_sx1272();
    if (!spix_write_reg( 0x01, 0x80 )) //  LoRa sleep
       printf("ERROR: spi operation failed: LoRa sleep (%02x)\n", spix_read_reg(0x01));

    if (!spix_write_reg( 0x01, 0x81 )) //  LoRa standby
       printf("ERROR: spi operation failed: standby (%02x)\n", spix_read_reg(0x01));

    if(!spix_write_reg( 0x0D, 0x80 )) // Set SPI FIFO ptr
       printf("ERROR: spi operation failed: set spi fifo ptr (%02x)\n", spix_read_reg(0x0D));

	spix_deselect();
	printf("End Init LoRa\n");
}

/*----------------------------------------------------------------------------------------------*/

void sx1272_config0() 		// change sx1272 device config
{
	spix_select(SX1272);
	// This is the only non-default value used in configuration
	unsigned char temp = spix_read_reg(0x1D); // RegModemConfig
	temp |= 0x02; 							// Set RxPayloadCrcOn
	spix_write_reg( 0x1D, temp ); 		// RegModemConfig with RxPayloadCrcOn set
	spix_deselect();
}

/*----------------------------------------------------------------------------------------------*/

void sx1272_set_RX(int len)
{	
	spix_select(SX1272);
    if(!spix_write_reg( 0x0C, 0x23 )) // Set LnaBoost=11 (150%)
       printf("ERROR: spi operation failed: set LnaBoost (%02x)\n", spix_read_reg(0x0C));
	spix_deselect();
}

/*----------------------------------------------------------------------------------------------*/

void sx1272_set_TX()
{	
	spix_select(SX1272);
    if (!spix_write_reg( 0x09, 0x8F )) // Set PaSelect
       printf("ERROR: spi operation failed: set PaSelect (%02x)\n", spix_read_reg(0x0D));

	spix_deselect();
}

/*----------------------------------------------------------------------------------------------*/

int sx1272_transmit(unsigned char data[], int len) 	// transmit a buffer
{
	spix_select(SX1272);

	int i;
   	unsigned long waiter=0;
	int monitor = 1;
	int ret;

	spix_write_reg( 0x0D, 0x80 ); // reset SPI ptr to start of Tx

	if(!spix_write_reg( 0x0E, 0x80 )) // Set TX FIFO ptr
		printf("ERROR: spi operation failed: set tx fifo ptr (%02x)\n", spix_read_reg(0x0E));

	spix_write_block(0x00, data, len);	// write data to Tx FIFO (0x80 | 0x00)
	
	if(!spix_write_reg(0x22, len)) 		// Set payload length
	  	printf("ERROR: spi operation failed: set payload length (%02x)\n", spix_read_reg(0x32));
      	
	spix_write_reg(0x12,0xFF); 			// clear all flags (including TX_DONE)

	printf("READING: - 0x01 reg: (%02x)\r\n", spix_read_reg(0x01));

		// Request transmit
	spix_write_reg( 0x01, 0x83 );
   //if(!spix_write_reg( 0x01, 0x83 ))
   //   printf("ERROR: spi operation failed: set TX mode (%02x)\n", spix_read_reg(0x01));
	//This flags an error, but the correct mode does get set.

   printf("READING: - 0x01 reg: (%02x)\r\n", spix_read_reg(0x01));

   unsigned char events=0x00;
   while (events == 0) 					// wait for event flag
      events = spix_read_reg(0x12);		//TODO: this is polling based. Should be event driven.

  printf("READING: - 0x12 reg: (%02x)\r\n", events);
  if(events ==0x08){ //TxDone
  	ret = 1;
  } else if(events == 0x80){
  	ret = 2;
  }
  else{
  	ret = 0;
  }

   if ((events & 0x08)==0x00) {
      printf("ERROR: TX_DONE not set in %d\n", events);
   }

	// DISPLAY
	spix_write_reg( 0x0D, 0x80 ); // reset SPI ptr to start of Tx
	for (i=0; i<len; i++)
		printf("%02x ", spix_read_reg(0x00)); // print tx fifo data

	printf("\n");

	spix_deselect();

	return ret;
}

/*----------------------------------------------------------------------------------------------*/

int sx1272_force_RxTimeout()
{
	//int ret = 0;

	spix_select(SX1272);

	spix_write_reg(0x1F,0x08); 			// set RegSymbTimeout to be something small
	printf("READING: - 0x1F reg: (%02x)\r\n", spix_read_reg(0x1F));

	spix_write_reg(0x12,0xFF); 			// clear all flags (including TX_DONE)		
	printf("READING: - 0x12 reg: (%02x)\r\n", spix_read_reg(0x12));

	spix_write_reg(0x01,0x86); 			// set to single receive mode
	printf("READING: - 0x01 reg: (%02x)\r\n", spix_read_reg(0x01));



	unsigned char events=0x00;
   	
   	while((events & 0x40) == 0){
   		events = PINE; 		
   	}

	printf("READING: - PINE: (%02x)\r\n", events);   	

   	/*while (events == 0) 					// wait for event flag
      events = spix_read_reg(0x12);

  	printf("READING: - 0x12 reg: (%02x)\r\n", events);
  	if ((events & 0x80)==0x00) {
      printf("ERROR: RxTimeout not set in %d\n", events);
      return 0;
   }
   else{
   	return 1;
   }*/

   if ((events & 0x40)==0x00) {
      printf("ERROR: interrupt not found %02x\n", events);
      return 0;
   }
   else{
   	return 1;
   }
}

unsigned char sx1272_receive()
{
	spix_select(SX1272);

	unsigned int base;
	unsigned int len;
	unsigned char rate;

    char *CRC_NONE = "None";
    char *CRC_ERROR = "Error";
    char *CRC_GOOD = "Good";
	char *crc;

	int i;

	spix_write_reg(0x12,0x50); // clear RX_DONE and ValidHeader

	// Reset RX FIFO base address to 0x00
	spix_write_reg( 0x0F, 0x00 ); // FifoRxBaseAddr

	if(!spix_write_reg( 0x01, 0x85 )) // RX CONTINUOUS
	  printf("ERROR: spi operation failed: rx mode (%02x)\n", spix_read_reg(0x01));

	while (spix_read_reg(0x12)==0x00); // wait for event flag

	if (!spix_write_reg( 0x12, 0x10 )) {
	  spix_write_reg(0x12, 0x10); // Clear ValidHeader
	  while (spix_read_reg(0x12)==0x00); // wait for event flag
	}

	if (spix_write_reg( 0x12, 0x40 ))
	  printf("ERROR: RX_DONE not set in %d\n", spix_read_reg(0x12));
	else 
	  printf("Receive successful:  ");
	

	if(!spix_write_reg( 0x01, 0x81 )) // LORA STANDBY
	  printf("ERROR: spi operation failed: LoRa standby mode (%02x)\n", spix_read_reg(0x01));

	// Zero the FifoAddrPtr
	if (!spix_write_reg(0x0D, 0x00))
		  printf("Error in zeroing RegFifoAddrPtr %02x\n", spix_read_reg(0x0D));

	spix_read_block(0x00, buffer); //get FIFO


	// INFORMATION TO RETURN

	base = spix_read_reg(0x10); // FifoRxCurrentAddr
	len = spix_read_reg(0x13); // FifoRxBytesNb

	rate = spix_read_reg(0x18); // RegModemStat
	rate = rate >> 5; // Shift the RxCodingRate bits to the lsb

	int psnr = spix_read_reg( 0x19 ); // RegPktSnrValue
	if ((psnr & 0x00000080) != 0)
	  psnr = psnr | 0xFFFFFF00;
	psnr = psnr/4;

	int prssi = spix_read_reg( 0x1A ); // RegPktRssiValue
	if (psnr>=0) 
	  prssi = (-139) + prssi;
	else
	  prssi = (-139) + prssi + psnr; // See page 112 of 1272 spec (rev 3)

	unsigned char temp = spix_read_reg(0x1C); // RegHopChannel
	if ((temp & 0x40) != 0x00) {
	  // CRC present
	  temp = spix_read_reg( 0x12 ); // RegIrqFlags
	  if ((temp & 0x20)!=0x00) {
		 crc = CRC_ERROR;
		 spix_write_reg( 0x12, 0x20 ); // Clear PayloadCrcError in ReqIrqFlags
	  }
	  else
		 crc = CRC_GOOD;
	}
	else {
	  crc = CRC_NONE;
	}

	// DISPLAY

	if (crc == CRC_GOOD) {
	  for (i=base; i<(base+len); i++) {
		 printf("%02x ", buffer[i]);
		 if ((i % 16)==15)
		    printf("\n");
	  }
	}
	printf("[len=%d,rate=%d,crc=%s,psnr=%d,prssi=%d]\n",
	  len, rate, crc, psnr, prssi);

	return len;

	spix_deselect();
}

/*----------------------------------------------------------------------------------------------*/
