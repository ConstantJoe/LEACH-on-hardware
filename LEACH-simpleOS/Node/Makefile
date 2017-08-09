PROJECT=leach

CC=avr-gcc
COPY=avr-objcopy
UPLOAD=avrdude
ELFFLAGS=-g -mmcu=atmega128rfa1
CFLAGS= $(ELFFLAGS) -Os 

DEPS=leds.h button.h Ctk_spi.h sx1272.h radio.h serial.h timer.h hw_timer.h

OBJ=leds.o button.o sx1272.o Ctk_spi.o
OBJ+= $(PROJECT).o

upload: $(PROJECT).hex
	sudo $(UPLOAD) -p m128rfa1 -U flash:w:$(PROJECT).hex -c jtag3 -v

$(PROJECT).hex: $(PROJECT).elf
	$(COPY) -j .text -j .data -O ihex $< $@

$(PROJECT).elf: $(OBJ)
	$(CC) $(ELFFLAGS) -o $@ $(OBJ) libOS.a

%.o: %.c $(DEPS)
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f *.o 
	rm -f *.hex
	rm -f *.elf
