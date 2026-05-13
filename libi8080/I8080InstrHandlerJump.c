/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 jump instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#include "I8080InstrHandlerJump.h"
#include "I8080System.h"
#include "I8080Logging.h"

I8080CycleCount
I8080InstrDispatchJump(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrCond_t    cond = I8080InstrExtShift(instr, 3, 5);
    I8080Addr_t         addr;
    bool                is_satisfied = false;
    const char          *jump_type = NULL;
    
    addr = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++) | (I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++) << 8);
    
    switch ( cond ) {
        case kI8080InstrCondOnNotZero:
            // Unconditional JMP when the lowest bit is set for this bit
            // pattern; conditionals if not set:
            if ( instr & 0b1 ) {
                is_satisfied = true;
                jump_type = "";
            } else {
                is_satisfied = ! sys8080->rgstrs.Z;
                jump_type = " on zero reset";
            }
            break;
        case kI8080InstrCondOnZero:
            is_satisfied = sys8080->rgstrs.Z;
            jump_type = " on zero set";
            break;
        case kI8080InstrCondOnNoCarry:
            is_satisfied = ! sys8080->rgstrs.CY;
            jump_type = " on carry reset";
            break;
        case kI8080InstrCondOnCarry:
            is_satisfied = sys8080->rgstrs.CY;
            jump_type = " on carry set";
            break;
        case kI8080InstrCondOnParityOdd:
            is_satisfied = ! sys8080->rgstrs.P;
            jump_type = " on parity odd";
            break;
        case kI8080InstrCondOnParityEven:
            is_satisfied = sys8080->rgstrs.P;
            jump_type = " on parity even";
            break;
        case kI8080InstrCondOnPlus:
            is_satisfied = ! sys8080->rgstrs.S;
            jump_type = " on sign reset (plus)";
            break;
        case kI8080InstrCondOnMinus:
            is_satisfied = sys8080->rgstrs.S;
            jump_type = " on sign set (minus)";
            break;
    }
    if ( is_satisfied ) {
        sys8080->rgstrs.PC = addr;
        DEBUG("Jump%s to $%04hX TAKEN", jump_type, addr);
    } else {
        DEBUG("Jump%s NOT TAKEN", jump_type);
    }
    return 10;
}

//

bool
I8080InstrFineGrainJump(
    I8080Instr_t    instr
)
{
    if ( instr == 0b11000011 ) return true;
    return ( (instr & 0b11000111) == 0b11000010 );
}

const I8080InstrHandler_t I8080InstrHandlerJump = {
        .name = "jump",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11000110,
        .pattern = 0b11000010,
        .fine_grain = I8080InstrFineGrainJump,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchJump,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchCall(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrCond_t    cond = I8080InstrExtShift(instr, 3, 5);
    I8080Addr_t         addr;
    bool                is_satisfied = false;
    const char          *call_type = NULL;
    
    addr = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++) | (I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++) << 8);
    
    switch ( cond ) {
        case kI8080InstrCondOnNotZero:
            is_satisfied = ! sys8080->rgstrs.Z;
            call_type = " on zero reset";
            break;
        case kI8080InstrCondOnZero:
            // Unconditional JMP when the lowest bit is set for this bit
            // pattern; conditionals if not set:
            if ( instr & 0b1 ) {
                is_satisfied = true;
                call_type = "";
            } else {
                is_satisfied = sys8080->rgstrs.Z;
                call_type = " on zero set";
            }
            break;
        case kI8080InstrCondOnNoCarry:
            is_satisfied = ! sys8080->rgstrs.CY;
            call_type = " on carry reset";
            break;
        case kI8080InstrCondOnCarry:
            is_satisfied = sys8080->rgstrs.CY;
            call_type = " on carry set";
            break;
        case kI8080InstrCondOnParityOdd:
            is_satisfied = ! sys8080->rgstrs.P;
            call_type = " on parity odd";
            break;
        case kI8080InstrCondOnParityEven:
            is_satisfied = sys8080->rgstrs.P;
            call_type = " on parity even";
            break;
        case kI8080InstrCondOnPlus:
            is_satisfied = ! sys8080->rgstrs.S;
            call_type = " on sign reset (plus)";
            break;
        case kI8080InstrCondOnMinus:
            is_satisfied = sys8080->rgstrs.S;
            call_type = " on sign set (minus)";
            break;
    }
    if ( is_satisfied ) {
        I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, (sys8080->rgstrs.PC >> 8));
        I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, (sys8080->rgstrs.PC & 0xFF));
        DEBUG("Call%s to $%04hX TAKEN, $%04hX pushed to stack @ $%04hX", call_type, addr, sys8080->rgstrs.PC, sys8080->rgstrs.SP + 1);
        sys8080->rgstrs.PC = addr;
        return 17;
    } else {
        DEBUG("Call%s NOT TAKEN", call_type);
    }
    return 11;
}

//

bool
I8080InstrFineGrainCall(
    I8080Instr_t    instr
)
{
    switch ( instr ) {
        case 0b11000100:
        case 0b11001100:
        case 0b11001101:
        case 0b11010100:
        case 0b11011100:
        case 0b11100100:
        case 0b11101100:
        case 0b11110100:
        case 0b11111100:
            return true;
    }
    return false;
}

const I8080InstrHandler_t I8080InstrHandlerCall = {
        .name = "call",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11000110,
        .pattern = 0b11000100,
        .fine_grain = I8080InstrFineGrainCall,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchCall,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchReturn(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrCond_t    cond = I8080InstrExtShift(instr, 3, 5);
    bool                is_satisfied = false;
    I8080CycleCount     cycles = 11;
    const char          *return_type = NULL;
    
    switch ( cond ) {
        case kI8080InstrCondOnNotZero:
            is_satisfied = ! sys8080->rgstrs.Z;
            return_type = " on zero reset";
            break;
        case kI8080InstrCondOnZero:
            // Unconditional RET when the lowest bit is set for this bit
            // pattern; conditionals if not set:
            if ( (instr & 0b1) ) {
                is_satisfied = true;
                cycles = 10;
                return_type = "";
            } else {
                is_satisfied = sys8080->rgstrs.Z;
                return_type = " on zero set";
            }
            break;
        case kI8080InstrCondOnNoCarry:
            is_satisfied = ! sys8080->rgstrs.CY;
            return_type = " on carry reset";
            break;
        case kI8080InstrCondOnCarry:
            is_satisfied = sys8080->rgstrs.CY;
            return_type = " on carry set";
            break;
        case kI8080InstrCondOnParityOdd:
            is_satisfied = ! sys8080->rgstrs.P;
            return_type = " on parity odd";
            break;
        case kI8080InstrCondOnParityEven:
            is_satisfied = sys8080->rgstrs.P;
            return_type = " on parity even";
            break;
        case kI8080InstrCondOnPlus:
            is_satisfied = ! sys8080->rgstrs.S;
            return_type = " on sign reset (plus)";
            break;
        case kI8080InstrCondOnMinus:
            is_satisfied = sys8080->rgstrs.S;
            return_type = " on sign set (minus)";
            break;
    }
    if ( is_satisfied ) {
        I8080Addr_t     addr;
        addr = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
        addr |= (I8080Addr_t)I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++) << 8;
        sys8080->rgstrs.PC = addr;
        DEBUG("Return%s to $%04hX TAKEN", return_type, addr);
        return cycles;
    } else {
        DEBUG("Return%s NOT TAKEN", return_type);
    }
    return 5;
}

//

bool
I8080InstrFineGrainReturn(
    I8080Instr_t    instr
)
{
    switch ( instr ) {
        case 0b11001001:
            return true;
    }
    return (instr & 0b11000111) == 0b11000000;
}

const I8080InstrHandler_t I8080InstrHandlerReturn = {
        .name = "return",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11000110,
        .pattern = 0b11000000,
        .fine_grain = I8080InstrFineGrainReturn,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchReturn,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchLoadPC(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    sys8080->rgstrs.PC = sys8080->rgstrs.H;
    sys8080->rgstrs.PC = (sys8080->rgstrs.PC << 8) | sys8080->rgstrs.L;
    return 5;
}

//

const I8080InstrHandler_t I8080InstrHandlerLoadPC = {
        .name = "load-pc",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11111111,
        .pattern = 0b11101001,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchLoadPC,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchRestart(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080Instr_t        slot = I8080InstrExtShift(instr, 3, 5);
    
    I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, (sys8080->rgstrs.PC >> 8));
    I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, (sys8080->rgstrs.PC & 0xFF));
    
    sys8080->rgstrs.PC = slot << 3;
    
    return 11;
}

//

const I8080InstrHandler_t I8080InstrHandlerRestart = {
        .name = "restart",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11000111,
        .pattern = 0b11000111,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchRestart,
        .next_handler = NULL
    };
