#include "Ctk_spi.h"

unsigned char buffer[256];

void sx1272_init(); // initialise spi and the sx1272 device

void sx1272_config0();		// change sx1272 device config
void sx1272_set_RX();

void sx1272_set_TX();

int sx1272_transmit(unsigned char data[], int len); 	// transmit a buffer
unsigned char sx1272_receive();

int sx1272_force_RxTimeout();