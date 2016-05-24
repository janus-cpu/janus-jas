#
# Config Makefile for the Janus Assembler
#

.PHONY = all execute clean

CC_FLAGS = -c -g -Wall -Werror -pedantic -O0 --std=c99
CC_WFLAGS = -c -g -O0 --std=c99
CC = gcc

CONFIG_EXE = instrsConfig
SRC_FILES = InstructionsConfig.c

all: execute

execute: $(CONFIG_EXE) instructions.jascfg
	@./$(CONFIG_EXE) ../src/InstructionList.h > /dev/null 2>&1

$(CONFIG_EXE): $(SRC_FILES)
	@$(CC) -o $(CONFIG_EXE) $(SRC_FILES)

clean:
	rm -f $(CONFIG_EXE)
