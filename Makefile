help:
	@echo 'Help details:'
	@echo 'hex: compile hex file'
	@echo 'flash: install hex file'
	@echo 'program: compile hex and install'

hex:
	avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c mike.cpp new.cpp
	avr-gcc -mmcu=atmega328p -o mike.elf mike.o new.o
	avr-objcopy -O ihex mike.elf mike.hex

clean:
	rm *.o
