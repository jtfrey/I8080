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

I8080CycleCount
I8080InstrDispatchSingleRegIncDec(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    uint8_t             prev = 0, byte = 0;
    uint8_t             src = I8080InstrExtShift(instr, 3, 5);
    uint8_t             OP, OP_lo;
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
                OP = sys8080->rgstrs.R[I8080RgstrIdx(src)], OP_lo = OP & 0xF;
                OP++, OP_lo++;
                DEBUG("Increment register %s = 0x%02hhX", I8080InstrRegisterNames[src], OP);
                sys8080->rgstrs.R[I8080RgstrIdx(src)] = OP & 0xFF;
                cycles = 5;
                break;
            }
            case kI8080InstrRegisterMem: {
                OP = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.HL), OP_lo = OP & 0xF;
                DEBUG("Byte 0x%02hhX read from HL=$%04hX", OP, sys8080->rgstrs.HL);
                OP++, OP_lo++;
                DEBUG("Increment byte = 0x%02hhX", OP);
                I8080MemWrite(sys8080->sysmem, sys8080->rgstrs.HL, OP);
                DEBUG("Byte 0x%02hhX written to HL=$%04hX", OP, sys8080->rgstrs.HL);
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
                OP = sys8080->rgstrs.R[I8080RgstrIdx(src)], OP_lo = OP & 0xF;
                OP--, OP_lo--;
                DEBUG("Decrement register %s = 0x%02hhX", I8080InstrRegisterNames[src], OP);
                sys8080->rgstrs.R[I8080RgstrIdx(src)] = OP & 0xFF;
                cycles = 5;
                break;
            }
            case kI8080InstrRegisterMem: {
                OP = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.HL), OP_lo = OP & 0xF;
                DEBUG("Byte 0x%02hhX read from HL=$%04hX", OP, sys8080->rgstrs.HL);
                OP--, OP_lo--;
                DEBUG("Decrement byte = 0x%02hhX", OP);
                I8080MemWrite(sys8080->sysmem, sys8080->rgstrs.HL, OP);
                DEBUG("Byte 0x%02hhX written to HL=$%04hX", OP, sys8080->rgstrs.HL);
                cycles = 10;
                break;
            }
        }
    }
    
    // Set flags:
    sys8080->rgstrs.Z = (OP == 0) ? 0b1 : 0b0;
    sys8080->rgstrs.S = (OP & 0b10000000) ? 0b1 : 0b0;
    sys8080->rgstrs.AC = (OP_lo & 0b00010000) ? 0b1 : 0b0;
    sys8080->rgstrs.P = I8080Parity(OP);
    
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
    uint16_t            A = sys8080->rgstrs.A;
    
    if ( instr == 0b00101111 ) {
        // CMA
        A ^= 0b11111111;
        DEBUG("1's complement 0x%02hhX = 0x%02hhX", sys8080->rgstrs.A, A); 
    } else {
        // DAA
        uint8_t        lo = A & 0xF;
        
        // Adjust low digit:
        if ( (lo > 9) || sys8080->rgstrs.AC ) {
            lo += 6, A += 6;
            sys8080->rgstrs.AC = (lo & ~0b1111) ? 0b1 : 0b0;
        }
        // Adjust high digit:
        if ( ((A & 0xF0) > 0x90) || sys8080->rgstrs.CY ) {
            A += 0x60;
            if ( A & ~0xFF ) sys8080->rgstrs.CY = 0b1;
        }
        
        DEBUG("Binary-coded decimal adjustment, 0x%02hhX => 0x%02hhX",
            sys8080->rgstrs.A , A);
        
        // Set flags:
        sys8080->rgstrs.Z = (A == 0) ? 0b1 : 0b0;
        sys8080->rgstrs.S = (A & 0b10000000) ? 0b1 : 0b0;
        sys8080->rgstrs.P = I8080Parity(A & 0xFF);
    }
    sys8080->rgstrs.A = A & 0xFF;
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
