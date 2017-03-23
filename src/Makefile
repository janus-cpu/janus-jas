#
# Source Makefile for the Janus Assembler
#

.PHONY = jas clean objects new

CC_FLAGS = -c -g -Wall -Werror -pedantic -O0 --std=c99
CC_WFLAGS = -c -g -O0 --std=c99
CC = gcc

LEX = flex

H_FILES = parser.h jas.h JasStrings.h \
		  Instruction.h Registers.h Labels.h InstructionList.h lexer.h
SRC_FILES = jas.c
OBJ_FILES = parser.o lexer.o Instruction.o Registers.o Labels.o

MAKE = make --no-print-directory

objects: $(OBJ_FILES) $(H_FILES)

.c.o:
	@echo "Compiling .c files .."
	$(CC) $(CC_FLAGS) ./$<
	@echo ""

clean:
	rm -f *.o # JasLexer.c

new:
	@$(MAKE) clean > /dev/null
	@$(MAKE)
