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
 
#include "I8080InstrHandlerDataMovement.h"
#include "I8080System.h"
#include "I8080Logging.h"

I8080CycleCount
I8080InstrDispatchDataMovementMov(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrRegister_t    dst = I8080InstrExtShift(instr, 3, 5),
                            src = I8080InstrExt(instr, 0, 2);
    I8080Instr_t            byte;
    I8080CycleCount         cycles = 5;
    
    switch ( src ) {
        case kI8080InstrRegisterB:
        case kI8080InstrRegisterC:
        case kI8080InstrRegisterD:
        case kI8080InstrRegisterE:
        case kI8080InstrRegisterH:
        case kI8080InstrRegisterL:
        case kI8080InstrRegisterA: {
            byte = sys8080->rgstrs.R[I8080RgstrIdx(src)];
            DEBUG("Loaded byte 0x%02hhX from register %s", byte, I8080InstrRegisterNames[src]);
            break;
        }
        case kI8080InstrRegisterMem: {
            if ( dst == kI8080InstrRegisterMem ) {
                // srt and dst of memory are illegal; guessing that
                // the decode takes ca. 4 cycles and we dump out right
                // after that:
                return 4;
            }
            byte = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.HL);
            DEBUG("Loaded byte 0x%02hhX from $%04hX", byte, sys8080->rgstrs.HL);
            cycles = 7;
            break;
        }
    }
    switch ( dst ) {
        case kI8080InstrRegisterB:
        case kI8080InstrRegisterC:
        case kI8080InstrRegisterD:
        case kI8080InstrRegisterE:
        case kI8080InstrRegisterH:
        case kI8080InstrRegisterL:
        case kI8080InstrRegisterA: {
            sys8080->rgstrs.R[I8080RgstrIdx(dst)] = byte;
            DEBUG("Moved byte 0x%02hhX to register %s", byte, I8080InstrRegisterNames[dst]);
            break;
        }
        case kI8080InstrRegisterMem: {
            I8080MemWrite(sys8080->sysmem, sys8080->rgstrs.HL, byte);
            DEBUG("Moved byte 0x%02hhX to $%04hX", byte, sys8080->rgstrs.HL);
            cycles = 7;
            break;
        }
    }
        
    
    return cycles;
}

//

bool
I8080InstrFineGrainDataMovementMov(
    I8080Instr_t    instr
)
{
    // "memory" as both src and dst is NOT allowed:
    return ( instr != 0b01110110 );
}

const I8080InstrHandler_t I8080InstrHandlerDataMovementMov = {
        .name = "data-movement-mov",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11000000,
        .pattern = 0b01000000,
        .fine_grain = I8080InstrFineGrainDataMovementMov,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchDataMovementMov,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchDataMovementAccum(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrRegisterPair_t    rp = I8080InstrExtShift(instr, 4, 4);
    
    if ( instr & 0b00001000 ) {
        // LDAX
        sys8080->rgstrs.A = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.RP[rp]);
        DEBUG("Loaded byte 0x%02hhX from %s=$%04hX",
            sys8080->rgstrs.A,
            I8080InstrRegisterPairNames[rp], sys8080->rgstrs.RP[rp]);
    } else {
        // STAX
        I8080MemWrite(sys8080->sysmem, sys8080->rgstrs.RP[rp], sys8080->rgstrs.A);
        DEBUG("Stored byte 0x%02hhX to %s=$%04hX",
            sys8080->rgstrs.A,
            I8080InstrRegisterPairNames[rp], sys8080->rgstrs.RP[rp]);
    }
    return 7;
}

//

const I8080InstrHandler_t I8080InstrHandlerDataMovementAccum = {
        .name = "data-movement-accum",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11100111,
        .pattern = 0b00000010,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchDataMovementAccum,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchDataMovementMVI(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrRegister_t    r = I8080InstrExtShift(instr, 3, 5);
    I8080Instr_t            byte = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++);
    I8080CycleCount         cycles = 7;
                                        
    DEBUG("Fetched immediate operand 0x%02hhX from $%04hX", byte, sys8080->rgstrs.PC - 1);
    
    switch ( r ) {
        case kI8080InstrRegisterMem:
            I8080MemWrite(sys8080->sysmem, sys8080->rgstrs.HL, byte);
            DEBUG("Moved byte 0x%02hhX to $%04hX", byte, sys8080->rgstrs.HL);
            cycles = 10;
            break;
        default:
            sys8080->rgstrs.R[I8080RgstrIdx(r)] = byte;
            DEBUG("Moved byte 0x%02hhX to register %s", byte, I8080InstrRegisterNames[r]);
            break;
    }

    return cycles;
}

//

const I8080InstrHandler_t I8080InstrHandlerDataMovementMVI = {
        .name = "data-movement-MVI",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11000111,
        .pattern = 0b00000110,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchDataMovementMVI,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchDataMovementLXI(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrRegisterPair_t    rp = I8080InstrExtShift(instr, 4, 5);
    I8080Instr_t                lo = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++),
                                hi = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++);
    
    DEBUG("Loaded two bytes 0x%02hhX, 0x%02hhX from $%04hX", lo, hi, sys8080->rgstrs.PC - 2);
    switch ( rp ) {
        case kI8080InstrRegisterPairSP:
            sys8080->rgstrs.SP = hi;
            sys8080->rgstrs.SP = (sys8080->rgstrs.SP << 8) | lo;
            DEBUG("Loaded address $%04hX into SP", sys8080->rgstrs.SP);
            break;
        default:
            sys8080->rgstrs.RP[rp] = hi;
            sys8080->rgstrs.RP[rp] = (sys8080->rgstrs.RP[rp] << 8) | lo;
            DEBUG("Loaded address $%04hX into %s", sys8080->rgstrs.RP[rp], I8080InstrRegisterPairNames[rp]);
            break;
    }

    return 10;
}

//

const I8080InstrHandler_t I8080InstrHandlerDataMovementLXI = {
        .name = "data-movement-LXI",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11001111,
        .pattern = 0b00000001,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchDataMovementLXI,
        .next_handler = NULL
    };

