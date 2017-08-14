#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>

#include "Ctk_spi.h"

unsigned char wflag, rflag;

void spix_load_lib()
{
}

void spix_select(unsigned char module)
{
	if (module == SX1272){
		wflag = 0x80;
		rflag = 0x00;
	}
	else if (module == PCF2123){
		wflag = 0x00;
		rflag = 0x80;
	}
	selected = module;
}

void spix_deselect()
{
	selected = 0;
}

// TRUE to select, FALSE to deselect
void spix_chipsel(unsigned char sel)
{
	if (selected == SX1272){		//select SX1272
		if (sel)
			PORTB &= ~(1<<CE_SX1272); // Drop CE to enable slave - mode 0
		else
			PORTB |= 1<<CE_SX1272; // raise to disable SPI slave - mode 0
	}
	else if (selected == PCF2123){	//select pcf2123
		if (sel)
			PORTE |= (1<<CE_PCF2123); //enable slave - mode 0
		else
			PORTE &= ~(1<<CE_PCF2123); //disable SPI slave - mode 0
	}
}

void spix_init_sx1272()
{
	spix_select(SX1272);
	// SS/CE, SCK, and MOSI as outputs, MISO as input
	DDRB = (1<<CE_SX1272) | (1<<PB5)| (1<<PB4)| (1<<PB2) | (1<<PB1); 
	PORTB = 1; // CE high=deselect only
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0); // clk/16=500kHz
	SPSR |= (1<<SPIF); // Clear int flag

	DDRE &= ~((1<<PE6) | (1<<PE7)); //temp  

	spix_chipsel(0);
}

void spix_init_pcf2123()
{
	spix_select(PCF2123);		//init pcf2123
	// SS/CE, SCLK, MOSI as outputs
	DDRB |= (1<<PB2) | (1<<PB1); // Output
    DDRE |= (1<<CE_PCF2123);
	// INT, CLKOUT (and MISO) as input
	DDRE &= ~((1<<PE7) | (1<<PE6));
	//PORTE = 0; // CE deselect only
	SPCR |= (1<<SPE) | (1<<MSTR) | (1<<SPR0); // clk/16=500kHz
	SPSR |= (1<<SPIF); // Clear int flag
	spix_chipsel(0);
}

void spix_xfer(unsigned char len, unsigned char *output, unsigned char *input)
{
	unsigned char i;
	spix_chipsel(1);
	for (i=0; i<len; i++) {
		SPDR = output[i]; // init slave spi
		while (!(SPSR && (1<<SPIF))); // wait for tx done
		input[i] = SPDR;
		SPSR |= (1<<SPIF); // Clear int flag
	//printf("%02x ", input[i]);
   }
	spix_chipsel(0);
}

bool spix_write_reg( unsigned char reg, unsigned char value )
{
	unsigned char reg_obuf[2], reg_ibuf[2];
	unsigned char rv;
	reg_obuf[0] = wflag | reg;
	reg_obuf[1] = value;
	spix_xfer( 2, reg_obuf, reg_ibuf );
	int oldvalue = spix_read_reg(reg);
	rv = spix_read_reg(reg);
	rv = spix_read_reg(reg);
	if (rv == value)
		return true;
	else
		return false;
}

unsigned char spix_read_reg( unsigned char reg)
{
   unsigned char reg_obuf[2], reg_ibuf[2];
   reg_obuf[0] = rflag | reg; // clear R/W flag
   reg_ibuf[1] = 0xFF; // dummy value
   spix_xfer( 2, reg_obuf, reg_ibuf );
   return reg_ibuf[1];
}

void spix_write_block(unsigned char reg, unsigned char value[], int len)
{
	int i;
	unsigned char reg_obuf[256], reg_ibuf[256];

	reg_obuf[0] = 0x80 | reg;

	for (i=0; i<len; i++)
		reg_obuf[1+i] = value[i];

	for (i=0; i<len; i++)
		reg_ibuf[1+i] = 0x5A; // mix of bits unlikely if spi transfer fails

	spix_xfer(len+1, reg_obuf, reg_ibuf);
}

void spix_read_block(unsigned char reg, unsigned char *buffer)
{
	int i;
	unsigned char reg_obuf[256], reg_ibuf[256];

	// Read 256 bytes from FIFO
	reg_obuf[0] = reg; 		// Select register 0 for read
	for (i=1; i<=256; i++)
		reg_obuf[i] = 0x00; 	// not used

	spix_xfer(255, reg_obuf, reg_ibuf);

	for (i=1; i<=256; i++)
		buffer[i-1] = reg_ibuf[i];
}
