/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 disassembly API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080DISASSEMBLER_H__
#define __I8080DISASSEMBLER_H__

#include "I8080Instructions.h"
#include "I8080Registers.h"
#include "I8080Memory.h"
#include "I8080TextBuffer.h"

typedef struct {
    bool                is_inte;
    I8080Addr_t         addr;
    I8080Instr_t        opcode;
    I8080Instr_t        operand1;
    I8080Instr_t        operand2;
    unsigned int        cycles;
} I8080FullInstrContext_t;

static inline
I8080Instr_t
I8080FullInstrContextFetch(
    I8080FullInstrContext_t *instr,
    I8080MemRef             sysmem,
    I8080Registers_t        *rgstrs
)
{
    instr->is_inte = false;
    instr->addr = rgstrs->PC;
    instr->operand1 = I8080MemRead(sysmem, rgstrs->PC + 1);
    instr->operand2 = I8080MemRead(sysmem, rgstrs->PC + 2);
    return (instr->opcode = I8080MemRead(sysmem, rgstrs->PC++));
}

void I8080FullInstrContextDisassembleToFile(FILE *fptr, I8080FullInstrContext_t *instr, I8080MemRef sysmem, I8080Registers_t *rgstrs);

void I8080FullInstrContextDisassembleToTextBuffer(I8080TextBufferRef tbuff, I8080FullInstrContext_t *instr, I8080MemRef sysmem, I8080Registers_t *rgstrs);

#endif /* __I8080DISASSEMBLER_H__ */
