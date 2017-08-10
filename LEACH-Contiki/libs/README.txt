to generate the a files:

avr-gcc -ggdb -mmcu=atmega128rfa1 -c $PROGRAM.c -o $PROGRAM.o
avr-ar -cvq l$PROGRAM.a $PROGRAM.o

