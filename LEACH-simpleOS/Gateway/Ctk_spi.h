#include <stdbool.h>

#define CE_SX1272 	0
#define CE_PCF2123 	4

#define SX1272 		1
#define PCF2123 	2

unsigned char selected;

void spix_load_lib();
void spix_select(unsigned char module);
void spix_chipsel(unsigned char sel);
void spix_init_sx1272();
void spix_init_pcf2123();

void spix_xfer(unsigned char len, unsigned char *output, unsigned char *input);

bool spix_write_reg(unsigned char reg, unsigned char value);		//write a single reg
unsigned char spix_read_reg(unsigned char reg); 					//read a single reg

void spix_write_block(unsigned char reg, unsigned char value[], int len);
void spix_read_block(unsigned char reg, unsigned char *buffer);
