#
# Source Makefile for the Janus Assembler
#

.PHONY = clean objects new

H_FILES = parser.h jas.h jas_strings.h jas_limits.h util.h \
		  instruction.h registers.h labels.h instruction_list.h lexer.h

objects: $(OBJ_FILES) $(H_FILES)

.c.o:
	@echo "Compiling .c files .."
	$(CC) $(CC_FLAGS) ./$<
	@echo ""

clean:
	rm -f *.o

new:
	@$(MAKE) clean > /dev/null
	@$(MAKE)
