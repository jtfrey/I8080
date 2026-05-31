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

/**
 * Lead-in context for 8080 instruction execution
 * In order to disassemble an instruction, we capture the PC, opcode,
 * and two possible operand bytes.
 */
typedef struct {
    bool                is_inte;    /*!< to be set externally, true indicates all that matters
                                         is the opcode and cycle count */
    I8080Addr_t         addr;       /*!< PC at which this instruction was read */
    I8080Instr_t        opcode;     /*!< instruction opcode */
    I8080Instr_t        operand1;   /*!< first byte following the opcode in RAM */
    I8080Instr_t        operand2;   /*!< second byte following the opcode in RAM */
    unsigned int        cycles;     /*!< to be set externally, cycle count of this instruction */
} I8080FullInstrContext_t;

/**
 * Fetch the next opcode and initialize a I8080FullInstrContext_t
 * The \p instr has its addr, opcode, and operand fields filled in.
 * @param instr             pointer to a \ref I8080FullInstrContext_t
 * @param sysmem            the system memory object
 * @param rgstrs            pointer to the emulated register set
 * @return                  the opcode of the fetched instruction
 */
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

/**
 * Disassemble the last instruction to a stdio file
 * Uses the fetch instruction context in \p instr and the current state
 * of \p sysmem and \p rgstrs to disassemble the last instruction and
 * write the summary to \p fptr.
 * @param fptr              a stdio file opened for write
 * @param instr             pointer to the \ref I8080FullInstrContext_t
 *                          for the executed instruction
 * @param sysmem            the system memory object
 * @param rgstrs            pointer to the emulated register set
 */
void I8080FullInstrContextDisassembleToFile(FILE *fptr, I8080FullInstrContext_t *instr, I8080MemRef sysmem, I8080Registers_t *rgstrs);

/**
 * Disassemble the last instruction to a text buffer
 * Variant of \ref I8080FullInstrContextDisassembleToFile() that writes
 * the summary to a \ref I8080TextBuffer.
 * @param tbuff             a text buffer object
 * @param instr             pointer to the \ref I8080FullInstrContext_t
 *                          for the executed instruction
 * @param sysmem            the system memory object
 * @param rgstrs            pointer to the emulated register set
 */
void I8080FullInstrContextDisassembleToTextBuffer(I8080TextBufferRef tbuff, I8080FullInstrContext_t *instr, I8080MemRef sysmem, I8080Registers_t *rgstrs);

#endif /* __I8080DISASSEMBLER_H__ */
