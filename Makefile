#
# Root Makefile for the Janus Assembler
#

.PHONY = cfg jas sources clean new

CC_FLAGS = -c -g -Wall -Werror -pedantic -O0 --std=c99
CC_WFLAGS = -c -g -O0 --std=c99
CC = gcc

SRC_FILES = jas.c
OBJ_FILES = JasParser.o JasLexer.o Instruction.o Registers.o Labels.o

MAKE = make --no-print-directory

all: cfg jas

jas: sources
	@echo "Final pass .."
	$(CC) -o jas $(addprefix ./src/, $(OBJ_FILES)) \
				 $(addprefix ./src/, $(SRC_FILES))
	@echo "Done."

cfg:
	@$(MAKE) -C config/ all

sources:
	@$(MAKE) -C src/ objects

clean:
	@$(MAKE) -C src/ clean
	rm -f jas a.out
	@echo "Clean."

new:
	@$(MAKE) clean > /dev/null
	@$(MAKE) all
