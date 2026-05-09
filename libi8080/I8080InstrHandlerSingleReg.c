/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 carry-bit instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#include "I8080InstrHandlerSingleReg.h"
#include "I8080System.h"
#include "I8080Logging.h"
#include "I8080BasicALOps.h"

I8080CycleCount
I8080InstrDispatchSingleRegIncDec(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    uint8_t             prev = 0, byte = 0;
    uint8_t             src = I8080InstrExtShift(instr, 3, 5);
    I8080ALU_t          alu = I8080ALUCreate(0, 0, false, false, true);
    I8080CycleCount     cycles;
    
    if ( I8080InstrExt(instr, 0, 0) == 0 ) {
        // INR
        switch ( src ) {
            case kI8080InstrRegisterB:
            case kI8080InstrRegisterC:
            case kI8080InstrRegisterD:
            case kI8080InstrRegisterE:
            case kI8080InstrRegisterH:
            case kI8080InstrRegisterL:
            case kI8080InstrRegisterA: {
                alu.OP1 = sys8080->rgstrs.R[I8080RgstrIdx(src)];
                I8080ALU1Inc(&alu);
                DEBUG("Increment register %s = 0x%02hhX", I8080InstrRegisterNames[src], alu.RES);
                sys8080->rgstrs.R[I8080RgstrIdx(src)] = alu.RES;
                cycles = 5;
                break;
            }
            case kI8080InstrRegisterMem: {
                alu.OP1 = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.HL);
                DEBUG("Byte 0x%02hhX read from HL=$%04hX", alu.OP1, sys8080->rgstrs.HL);
                I8080ALU1Inc(&alu);
                DEBUG("Increment byte = 0x%02hhX", I8080InstrRegisterNames[src], alu.RES);
                I8080MemWrite(sys8080->sysmem, sys8080->rgstrs.HL, byte);
                DEBUG("Byte 0x%02hhX written to HL=$%04hX", alu.OP1, sys8080->rgstrs.HL);
                cycles = 10;
                break;
            }
        }
    } else {
        // DCR
        switch ( src ) {
            case kI8080InstrRegisterB:
            case kI8080InstrRegisterC:
            case kI8080InstrRegisterD:
            case kI8080InstrRegisterE:
            case kI8080InstrRegisterH:
            case kI8080InstrRegisterL:
            case kI8080InstrRegisterA: {
                alu.OP1 = sys8080->rgstrs.R[I8080RgstrIdx(src)];
                I8080ALU1Dec(&alu);
                DEBUG("Decrement register %s = 0x%02hhX", I8080InstrRegisterNames[src], alu.RES);
                sys8080->rgstrs.R[I8080RgstrIdx(src)] = alu.RES;
                cycles = 5;
                break;
            }
            case kI8080InstrRegisterMem: {
                alu.OP1 = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.HL);
                DEBUG("Byte 0x%02hhX read from HL=$%04hX", alu.OP1, sys8080->rgstrs.HL);
                I8080ALU1Dec(&alu);
                DEBUG("Decrement byte = 0x%02hhX", I8080InstrRegisterNames[src], alu.RES);
                I8080MemWrite(sys8080->sysmem, sys8080->rgstrs.HL, byte);
                DEBUG("Byte 0x%02hhX written to HL=$%04hX", alu.OP1, sys8080->rgstrs.HL);
                cycles = 10;
                break;
            }
        }
    }
    
    // Set flags:
    sys8080->rgstrs.Z = (byte == 0);
    sys8080->rgstrs.S = ((byte & 0b10000000) == 0b10000000);
    sys8080->rgstrs.AC = ((prev & 0b00001000) && ! (byte & 0b00001000));
    sys8080->rgstrs.P = I8080InstrBitParity(byte);
    
    return cycles;
}

//

const I8080InstrHandler_t I8080InstrHandlerSingleRegIncDec = {
        .name = "single-register-incdec",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11000110,
        .pattern = 0b00000100,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchSingleRegIncDec,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchSingleRegAccum(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    if ( instr == 0b00101111 ) {
        // CMA
        sys8080->rgstrs.A ^= 0b11111111;
    } else {
        // DAA
        uint8_t     prev, A = sys8080->rgstrs.A;
        
        if ( ((A & 0b00001111) > 6) || sys8080->rgstrs.AC ) {
            prev = A;
            A += 6;
            sys8080->rgstrs.AC = ((prev & 0b00001000) && ! (A & 0b00001000));
        } else {
            sys8080->rgstrs.AC = 0;
        }
        if ( ((A & 0b11110000) > (9 << 4)) || sys8080->rgstrs.CY ) {
            prev = A;
            A += (6 << 4);
            sys8080->rgstrs.CY = ((prev & 0b10000000) && ! (A & 0b10000000));
        } else {
            sys8080->rgstrs.CY = 0;
        }
        
        // Write-back to accumulator:
        sys8080->rgstrs.A = A;
        
        // Set flags:
        sys8080->rgstrs.Z = (A == 0);
        sys8080->rgstrs.S = ((A & 0b10000000) == 0b10000000);
        sys8080->rgstrs.P = I8080InstrBitParity(A);
    }
    return 4;
}

//

const I8080InstrHandler_t I8080InstrHandlerSingleRegAccum = {
        .name = "single-register-accum",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11110111,
        .pattern = 0b00100111,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchSingleRegAccum,
        .next_handler = NULL
    };
