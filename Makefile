#
# Root Makefile for the Janus Assembler
#

.PHONY = sources clean new

CC_FLAGS = -c -Wall -Werror -pedantic --std=c99
CC = gcc
SRC_FILES = jas.c
OBJ_FILES = parser.o lexer.o instruction.o registers.o labels.o output.o \
			jas_limits.o util.o
MAKE = make --no-print-directory

ifeq ($(opt),true)
	CC_FLAGS += -O3
else
	CC_FLAGS += -g
endif

# Export for child make
export CC_FLAGS
export CC
export SRC_FILES
export OBJ_FILES
export MAKE

all: jas

jas: sources
	@echo "Final pass .."
	$(CC) -o jas $(addprefix ./src/, $(OBJ_FILES)) \
				 $(addprefix ./src/, $(SRC_FILES))
	@echo "Done."

sources:
	@$(MAKE) -C src/ objects

clean:
	@$(MAKE) -C src/ clean
	rm -f jas a.out
	@echo "Clean."

new:
	@$(MAKE) clean > /dev/null
	@$(MAKE) all
