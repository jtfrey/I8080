/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 accumulator instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#include "I8080InstrHandlerAccum.h"
#include "I8080System.h"
#include "I8080Logging.h"

I8080CycleCount
I8080InstrDispatchAccum(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrRegister_t    r = I8080InstrExt(instr, 0, 2);
    I8080InstrALUOp_t       op = I8080InstrExtShift(instr, 3, 5);
    I8080CycleCount         cycles = 4;
    uint16_t                A = sys8080->rgstrs.A, OP2,
                            A_lo = A & 0xF, OP2_lo;
    bool                    set_A = true;
#ifndef I8080_STRIP_DEBUGGING
    char                    opstr;
    const char              *debug_fmt = "Accumulator 0x%1$02hhX %2$c 0x%3$02hhX = 0x%4$02hhX (%5$X%6$X%7$X%8$X%9$X%10$X)\n";
    const char              *cy_debug_fmt = "Accumulator 0x%1$02hhX %2$c 0x%3$02hhX + %11$X= 0x%4$02hhX (%5$X%6$X%7$X%8$X%9$X%10$X)\n";
    int                     CY = sys8080->rgstrs.CY;
#   define SET_OPSTR(C)     opstr = C;
#   define SET_DEBUG_FMT(S) debug_fmt = S;
#else
#   define SET_OPSTR(C)
#   define SET_DEBUG_FMT(S)
#endif
    
    switch ( r ) {
        case kI8080InstrRegisterB:
        case kI8080InstrRegisterC:
        case kI8080InstrRegisterD:
        case kI8080InstrRegisterE:
        case kI8080InstrRegisterH:
        case kI8080InstrRegisterL:
        case kI8080InstrRegisterA: {
            OP2 = sys8080->rgstrs.R[I8080RgstrIdx(r)];
            DEBUG("Fetched operand 0x%02hhX from register %s", OP2, I8080InstrRegisterNames[r]);
            break;
        }
        case kI8080InstrRegisterMem: {
            OP2 = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.HL);
            DEBUG("Fetched operand 0x%02hhX from $%04hX", OP2, sys8080->rgstrs.HL);
            cycles = 7;
            break;
        }
    }
    
    switch ( op ) {
        case kI8080InstrALUOpAddC:
            OP2_lo = OP2 & 0xF;
            A += OP2 + sys8080->rgstrs.CY;
            A_lo += OP2_lo + sys8080->rgstrs.CY;
            SET_OPSTR('+');
            SET_DEBUG_FMT(cy_debug_fmt);
            break;
        case kI8080InstrALUOpAdd:
            OP2_lo = OP2 & 0xF;
            A += OP2;
            A_lo += OP2_lo;
            SET_OPSTR('+');
            break;
        
        case kI8080InstrALUOpSubC: {
#ifndef I8080_STRIP_DEBUGGING
            uint16_t OP2_copy = OP2;
#endif
            OP2 = ~(OP2 + sys8080->rgstrs.CY) + 1;
            OP2_lo = OP2 & 0xF;
            A += OP2;
            A_lo += OP2_lo;
#ifndef I8080_STRIP_DEBUGGING
            OP2 = OP2_copy;
#endif
            SET_OPSTR('-');
            SET_DEBUG_FMT(cy_debug_fmt);
            break;
        }
        case kI8080InstrALUOpCmp:
            set_A = false;
        case kI8080InstrALUOpSub: {
#ifndef I8080_STRIP_DEBUGGING
            uint16_t OP2_copy = OP2;
#endif
            OP2 = ~OP2 + 1;
            OP2_lo = OP2 & 0xF;
            A += OP2;
            A_lo += OP2_lo;
#ifndef I8080_STRIP_DEBUGGING
            OP2 = OP2_copy;
#endif
            SET_OPSTR('-');
            break;
        }
            
        case kI8080InstrALUOpAnd:
            A &= OP2;
#ifdef I8080_AND_CLEARS_AC
            A_lo = 0;
#else
            // OR of bit 3 shifts to position 4 to detect AC
            A_lo = (A | OP2) << 1;
#endif
            SET_OPSTR('&');
            break;
            
        case kI8080InstrALUOpXor:
            A ^= OP2;
            A_lo = 0;
            SET_OPSTR('^');
            break;
            
        case kI8080InstrALUOpOr:
            A |= OP2;
            A_lo = 0;
            SET_OPSTR('|');
            break;
    }
    sys8080->rgstrs.Z = (A == 0) ? 0b1 : 0b0;
    sys8080->rgstrs.S = (A & 0b10000000) ? 0b1 : 0b0;
    sys8080->rgstrs.AC = (A_lo & 0b00010000) ? 0b1 : 0b0;
    sys8080->rgstrs.P = I8080Parity(A & 0xFF);
    sys8080->rgstrs.CY = (A & 0b100000000) ? 0b1 : 0b0;
    DEBUG(debug_fmt,
            sys8080->rgstrs.A, opstr, OP2, A, sys8080->rgstrs.Z, sys8080->rgstrs.S,
            sys8080->rgstrs.AC, sys8080->rgstrs.P, sys8080->rgstrs.CY, CY);
    if ( set_A ) sys8080->rgstrs.A = A & 0xFF;
    
    return cycles;
#undef SET_OPSTR
#undef SET_DEBUG_FMT
}

//

const I8080InstrHandler_t I8080InstrHandlerAccum = {
        .name = "accum",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11000000,
        .pattern = 0b10000000,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchAccum,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchAccumRotate(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrRotateOp_t    op = I8080InstrExtShift(instr, 3, 4);
    uint16_t                A = sys8080->rgstrs.A, CY = sys8080->rgstrs.CY;
#ifndef I8080_STRIP_DEBUGGING
    const char*             opstr;
#   define SET_OPSTR(S)     opstr = S;
#else
#   define SET_OPSTR(S)
#endif
    
    switch ( op ) {
        case kI8080InstrRotateOpLeft:
            CY = (A & 0b10000000) ? 0b1 : 0b0;
            A = (A << 1) | CY;
            SET_OPSTR("RLC");
            break;
        case kI8080InstrRotateOpRight:
            CY = (A & 0b1);
            A = (A >> 1) | (CY << 7);
            SET_OPSTR("RRC");
            break;
        case kI8080InstrRotateOpLeftThruCarry:
            A = (A << 1) | sys8080->rgstrs.CY;
            CY = (A & 0b100000000);
            SET_OPSTR("RAL");
            break;
        case kI8080InstrRotateOpRightThruCarry:
            CY = (A & 0b1);
            A = (A >> 1) | (sys8080->rgstrs.CY << 7);
            SET_OPSTR("RAR");
            break;
    }
    A &= 0xFF;
    DEBUG("Accumulator 0x%1$02hhX %2$s = 0x%3$02hhX (CY=%4$X)\n",
            sys8080->rgstrs.A, opstr, A, CY);
    sys8080->rgstrs.A = A, sys8080->rgstrs.CY = CY ? 0b1 : 0b0;
    return 4;
#undef SET_OPSTR
}

//

const I8080InstrHandler_t I8080InstrHandlerAccumRotate = {
        .name = "accum-rotate",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11100111,
        .pattern = 0b00000111,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchAccumRotate,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchAccumImmed(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrALUOp_t       op = I8080InstrExtShift(instr, 3, 5);
    uint16_t                A = sys8080->rgstrs.A, OP2 = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++),
                            A_lo = A & 0xF, OP2_lo = OP2 & 0xF;
    bool                    set_A = true;
#ifndef I8080_STRIP_DEBUGGING
    char                    opstr;
    const char              *debug_fmt = "Accumulator 0x%1$02hhX %2$c 0x%3$02hhX = 0x%4$02hhX (%5$X%6$X%7$X%8$X%9$X%10$X)\n";
    const char              *cy_debug_fmt = "Accumulator 0x%1$02hhX %2$c 0x%3$02hhX + %11$X= 0x%4$02hhX (%5$X%6$X%7$X%8$X%9$X%10$X)\n";
    int                     CY = sys8080->rgstrs.CY;
#   define SET_OPSTR(C)     opstr = C;
#   define SET_DEBUG_FMT(S) debug_fmt = S;
#else
#   define SET_OPSTR(C)
#   define SET_DEBUG_FMT(S)
#endif
                                        
    DEBUG("Fetched immediate operand 0x%02hhX from $%04hX", OP2, sys8080->rgstrs.PC - 1);
    
    switch ( op ) {
        case kI8080InstrALUOpAddC:
            A += OP2 + sys8080->rgstrs.CY;
            A_lo += OP2_lo + sys8080->rgstrs.CY;
            SET_OPSTR('+');
            SET_DEBUG_FMT(cy_debug_fmt);
            break;
        case kI8080InstrALUOpAdd:
            A += OP2;
            A_lo += OP2_lo;
            SET_OPSTR('+');
            break;
        
        case kI8080InstrALUOpSubC: {
#ifndef I8080_STRIP_DEBUGGING
            uint16_t OP2_copy = OP2;
#endif
            OP2 = ~(OP2 + sys8080->rgstrs.CY) + 1;
            A += OP2;
            A_lo += OP2_lo;
#ifndef I8080_STRIP_DEBUGGING
            OP2 = OP2_copy;
#endif
            SET_OPSTR('-');
            SET_DEBUG_FMT(cy_debug_fmt);
            break;
        }
        case kI8080InstrALUOpCmp:
            set_A = false;
        case kI8080InstrALUOpSub: {
#ifndef I8080_STRIP_DEBUGGING
            uint16_t OP2_copy = OP2;
#endif
            OP2 = ~OP2 + 1;
            A += OP2;
            A_lo += OP2_lo;
#ifndef I8080_STRIP_DEBUGGING
            OP2 = OP2_copy;
#endif
            SET_OPSTR('-');
            break;
        }
            
        case kI8080InstrALUOpAnd:
            A &= OP2;
#ifdef I8080_AND_CLEARS_AC
            A_lo = 0;
#else
            // OR of bit 3 shifts to position 4 to detect AC
            A_lo = (A | OP2) << 1;
#endif
            SET_OPSTR('&');
            break;
            
        case kI8080InstrALUOpXor:
            A ^= OP2;
            A_lo = 0;
            SET_OPSTR('^');
            break;
            
        case kI8080InstrALUOpOr:
            A |= OP2;
            A_lo = 0;
            SET_OPSTR('|');
            break;
    }
    sys8080->rgstrs.Z = (A == 0) ? 0b1 : 0b0;
    sys8080->rgstrs.S = (A & 0b10000000) ? 0b1 : 0b0;
    sys8080->rgstrs.AC = (A_lo & 0b00010000) ? 0b1 : 0b0;
    sys8080->rgstrs.P = I8080Parity(A & 0xFF);
    sys8080->rgstrs.CY = (A & 0b100000000) ? 0b1 : 0b0;
    DEBUG(debug_fmt,
            sys8080->rgstrs.A, opstr, OP2, A, sys8080->rgstrs.Z, sys8080->rgstrs.S,
            sys8080->rgstrs.AC, sys8080->rgstrs.P, sys8080->rgstrs.CY, CY);
    if ( set_A ) sys8080->rgstrs.A = A & 0xFF;
            
    return 7;
#undef SET_OPSTR
#undef SET_DEBUG_FMT
}

//

const I8080InstrHandler_t I8080InstrHandlerAccumImmed = {
        .name = "accum-immediate",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11000111,
        .pattern = 0b11000110,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchAccumImmed,
        .next_handler = NULL
    };
