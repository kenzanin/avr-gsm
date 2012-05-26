# AVRLIB
OBJS  = avrlib/buffer.o avrlib/uart.o avrlib/rprintf.o

# LIBs
OBJS += lib/lcd.o lib/gsm.o main.o

CC = avr-gcc
MCU = atmega32
LFUSE = 0xE2
HFUSE = 0xD1
EFUSE = 0xFF
LOCKFUSE = 0x3C
INCDIR = -I. -I./avrlib -I./lib

CPFLAGS = -O4 -Wl,-u,vfprintf -lprintf_flt -lm

LF="\n"; CR="\r"
INVT="\033[7m"
NORM="\033[0m"
BOLD="\033[1m"

BLACK_F="\033[30m"
BLACK_B="\033[40m"
RED_F="\033[31m"
RED_B="\033[41m"
GREEN_F="\033[32m"
GREEN_B="\033[42m"
YELLOW_F="\033[33m"
YELLOW_B="\033[43m"
BLUE_F="\033[34m"
BLUE_B="\033[44m"
MAGENTA_F="\033[35m"
MAGENTA_B="\033[45m"
CYAN_F="\033[36m"
CYAN_B="\033[46m"
WHITE_F="\033[37m"
WHITE_B="\033[47m"
HL = @echo   $(BLUE_F) "-----------------------------------" $(NORM)

all: $(OBJS)
# Link objects
	@echo   $(RED_B)$(BLACK_F) LD $(NORM) $(OBJS)
	@avr-gcc $(OBJS) -o output.o $(INCDIR) -mmcu=$(MCU) $(CPFLAGS) -Wall
	$(HL)
# Convert object to intel HEX format
	@echo   $(RED_B)$(BLACK_F) HEX $(NORM) output.hex
	@avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock output.o output.hex 
# create eeprom hex
	@echo   $(RED_B)$(BLACK_F) EEPROM $(NORM) output.eep
	@avr-objcopy -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex output.o output.eep
	$(HL)
# Print output size
	@echo   $(RED_B)$(BLACK_F) SIZE $(NORM)
	@echo   $(YELLOW_F)
	@avr-size -C --mcu=$(MCU) output.o
	@echo $(NORM)
# Print Date & Time
	@echo   $(RED_B)$(BLACK_F) `date` $(NORM)
# Print Done
	$(HL)
	@echo [ Done ]

flash:
	sudo avrdude -c usbasp -p $(MCU) -U flash:w:output.hex
#Log in file
	@echo `date` >> flashlog.txt
# Print Date & Time
	@echo   $(RED_B)$(BLACK_F) `date` $(NORM)

eeprom:
	sudo avrdude -c usbasp -p $(MCU) -U eeprom:w:output.eep

report:
	@echo   $(YELLOW_F)
	@echo "Number of flashes:\t" `cat flashlog.txt | wc -l`
	@echo "Number of code lines:\t" `find . -name '*.c' | xargs cat | wc -l`
	@echo $(NORM)

fuse:
	sudo avrdude -c usbasp -p $(MCU) -U lfuse:w:$(LFUSE):m
	sudo avrdude -c usbasp -p $(MCU) -U hfuse:w:$(HFUSE):m

lock:
	sudo avrdude -c usbasp -p $(MCU) -U lock:w:$(LOCKFUSE):m

rfuse:
	sudo avrdude -cusbasp -p $(MCU) -U lfuse:r:lfuse.bin:r
	sudo avrdude -cusbasp -p $(MCU) -U hfuse:r:hfuse.bin:r
	hexdump -C lfuse.bin
	hexdump -C hfuse.bin

	
%.o: %.c
	@echo   $(RED_B)$(BLACK_F) CC $(NORM) $@
	@$(CC) -c $< -Wall $(INCDIR) -mmcu=$(MCU) $(CPFLAGS) -o$@
	$(HL)

.S.o:
	$(CC) -c $< -x assembler-with-cpp -Wall $(INCDIR) -mmcu=$(MCU) $(CPFLAGS) -o$@

.c.s:
	$(CC) -c $< -Wall $(INCDIR) -mmcu=$(MCU) $(CPFLAGS) -o$@
	
clean:
	rm -rfv *.o *.hex avrlib/*.o lib/*.o

