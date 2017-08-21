PROJECT=leach_c_gateway

CC=avr-gcc
COPY=avr-objcopy
UPLOAD=avrdude
ELFFLAGS=-g -mmcu=atmega128rfa1
CFLAGS= $(ELFFLAGS) -Os 

DEPS=$(PROJECT).h leds.h button.h radio.h serial.h hw_timer.h simple_os.h message_structs.h

OBJ=leds.o button.o radio.o hw_timer.o serial.o simple_os.o
OBJ+= $(PROJECT).o

upload: $(PROJECT).hex
	sudo $(UPLOAD) -p m128rfa1 -U flash:w:$(PROJECT).hex -c jtag3 -v

$(PROJECT).hex: $(PROJECT).elf
	$(COPY) -j .text -j .data -O ihex $< $@

$(PROJECT).elf: $(OBJ)
	$(CC) $(ELFFLAGS) -o $@ $(OBJ)

%.o: %.c $(DEPS)
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f *.o 
	rm -f *.hex
	rm -f *.elf
