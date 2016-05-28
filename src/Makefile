#
# Source Makefile for the Janus Assembler
#

.PHONY = jas clean objects new

CC_FLAGS = -c -g -Wall -Werror -pedantic -O0 --std=c99
CC_WFLAGS = -c -g -O0 --std=c99
CC = gcc

LEX = flex

H_FILES = JasParser.h jas.h JasStrings.h \
		  Instruction.h Registers.h Labels.h InstructionList.h
SRC_FILES = jas.c
OBJ_FILES = JasParser.o JasLexer.o Instruction.o Registers.o Labels.o

MAKE = make --no-print-directory

objects: $(OBJ_FILES) $(H_FILES)

.c.o:
	@echo "Compiling .c files .."
	$(CC) $(CC_FLAGS) ./$<
	@echo ""

.l.o:
	@echo "Reading flex rules .."
	$(LEX) -o $(basename $<).c $<
	$(CC) $(CC_WFLAGS) $(basename $<).c
	rm -f $(basename $<).c
	@echo ""

clean:
	rm -f *.o JasLexer.c

new:
	@$(MAKE) clean > /dev/null
	@$(MAKE)
