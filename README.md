# jas

## About
The Janus Assembler (`jas`) uses Flex and C to generate machine code for the
[`vesta`](https://github.com/janus-cpu/janus-vesta) virtual machine.

## How does it?
`jas` achieves code generation in three steps (as of now):
 1. Lexical Analysis - syntax checks, load information about instructions and
    labels.
 2. Semantic Analysis - instruction/operand type checking, size agreement, etc.
 3. Name Resolution and Linking\* - resolve undefined labels, etc.

## Development files
If you want to check out `jas` for yourself, you'll need the following installed:
 + `make`
 + `flex`
 + `gcc`

Once you're sure that you do, clone the repo:
```
$ git clone https://github.com/pencels/jas.git
```
and `make` the `jas` executable. Of course, you'd also need to have
[`vesta`](https://github.com/janus-cpu/janus-vesta) as well.

### Some Makefile targets
 + `make` - compile the assembler
 + `make clean` - removes any generated files
 + `make new` - runs `make clean` then `make` again

\* Note: not implemented yet.
