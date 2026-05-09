/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 stack instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#include "I8080InstrHandlerRegPairs.h"
#include "I8080System.h"
#include "I8080Logging.h"

I8080CycleCount
I8080InstrDispatchRegPairStack(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrRegisterPair_t    pair = I8080InstrExtShift(instr, 4, 5);
    I8080CycleCount             cycles;
    
    if ( instr & 0b00000100 ) {
        // PUSH
        switch ( pair ) {
            case kI8080InstrRegisterPairBC:
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.B);
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.C);
                DEBUG("Pushed BC=0x%04hX to the stack at $%04hX",
                    sys8080->rgstrs.BC, sys8080->rgstrs.SP + 2);
                break;
            case kI8080InstrRegisterPairDE:
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.D);
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.E);
                DEBUG("Pushed DE=0x%04hX to the stack at $%04hX",
                    sys8080->rgstrs.DE, sys8080->rgstrs.SP + 2);
                break;
            case kI8080InstrRegisterPairHL:
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.H);
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.L);
                DEBUG("Pushed HL=0x%04hX to the stack at $%04hX",
                    sys8080->rgstrs.HL, sys8080->rgstrs.SP + 2);
                break;
            case kI8080InstrRegisterPairPSW:
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.A);
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.F);
                DEBUG("Pushed PSW=0x%04hX to the stack at $%04hX",
                    sys8080->rgstrs.PSW, sys8080->rgstrs.SP + 2);
                break;
        }
        cycles = 11;
    } else {
        // POP
        switch ( pair ) {
            case kI8080InstrRegisterPairBC:
                sys8080->rgstrs.C = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                sys8080->rgstrs.C = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                DEBUG("Popped BC=0x%04hX for the stack at $%04hX",
                    sys8080->rgstrs.BC, sys8080->rgstrs.SP - 2);
                break;
            case kI8080InstrRegisterPairDE:
                sys8080->rgstrs.E = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                sys8080->rgstrs.D = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                DEBUG("Popped DE=0x%04hX for the stack at $%04hX",
                    sys8080->rgstrs.DE, sys8080->rgstrs.SP - 2);
                break;
            case kI8080InstrRegisterPairHL:
                sys8080->rgstrs.L = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                sys8080->rgstrs.H = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                DEBUG("Popped HL=0x%04hX for the stack at $%04hX",
                    sys8080->rgstrs.HL, sys8080->rgstrs.SP - 2);
                break;
            case kI8080InstrRegisterPairPSW:
                sys8080->rgstrs.F = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                sys8080->rgstrs.A = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                DEBUG("Popped PSW=0x%04hX for the stack at $%04hX",
                    sys8080->rgstrs.PSW, sys8080->rgstrs.SP - 2);
                break;
        }
        cycles = 10;
    }
    return cycles;
}

//

const I8080InstrHandler_t I8080InstrHandlerRegPairsStack = {
        .name = "register-pairs-stack",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11001011,
        .pattern = 0b11000001,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchRegPairStack,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchRegPairsArith(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrRegisterPair_t    pair = I8080InstrExtShift(instr, 4, 5);
    I8080CycleCount             cycles;
    
    switch ( instr & 0b00001110 ) {
        case 0b00001000: {
            // DAD
            uint32_t            base = sys8080->rgstrs.HL,
                                offset;
                                
            switch ( pair ) {
                case kI8080InstrRegisterPairBC:
                    offset = sys8080->rgstrs.BC;
                    break;
                case kI8080InstrRegisterPairDE:
                    offset = sys8080->rgstrs.DE;
                    break;
                case kI8080InstrRegisterPairHL:
                    offset = sys8080->rgstrs.HL;
                    break;
                case kI8080InstrRegisterPairSP:
                    offset = sys8080->rgstrs.SP;
                    break;
            }
            base += offset;
            DEBUG("Double-add HL=0x%04hX + %s=0x%04hX = 0x%04hX",
                sys8080->rgstrs.HL, I8080InstrRegisterPairNames[pair], offset, base & 0xFFFF);
            sys8080->rgstrs.HL = base & 0xFFFF;
            sys8080->rgstrs.CY = (base & ~0xFFFFFFFF) ? 0b1 : 0b0;
            cycles = 10;
            break;
        }
        case 0b00001010: {
            // INX
            uint16_t        result;
            
            switch ( pair ) {
                case kI8080InstrRegisterPairBC:
                    result = ++sys8080->rgstrs.BC;
                    break;
                case kI8080InstrRegisterPairDE:
                    result = ++sys8080->rgstrs.DE;
                    break;
                case kI8080InstrRegisterPairHL:
                    result = ++sys8080->rgstrs.HL;
                    break;
                case kI8080InstrRegisterPairSP:
                    result = ++sys8080->rgstrs.SP;
                    break;
            }
            DEBUG("Increment %s = 0x%04hX",I8080InstrRegisterPairNames[pair], result);
            cycles = 5;
            break;
        }
        case 0b00000010: {
            // DCX
            uint16_t        result;
            
            switch ( pair ) {
                case kI8080InstrRegisterPairBC:
                    result = --sys8080->rgstrs.BC;
                    break;
                case kI8080InstrRegisterPairDE:
                    result = --sys8080->rgstrs.DE;
                    break;
                case kI8080InstrRegisterPairHL:
                    result = --sys8080->rgstrs.HL;
                    break;
                case kI8080InstrRegisterPairSP:
                    result = --sys8080->rgstrs.SP;
                    break;
            }
            DEBUG("Decrement %s = 0x%04hX",I8080InstrRegisterPairNames[pair], result);
            cycles = 5;
            break;
        }
    }
    return cycles;
}

//

bool
I8080InstrFineGrainRegPairsArith(
    I8080Instr_t    instr
)
{
    switch ( instr & 0b11001111 ) {
        case 0b00001001:
        case 0b00000011:
        case 0b00001011:
            return true;
    }
    return false;
}

const I8080InstrHandler_t I8080InstrHandlerRegPairsArith = {
        .name = "register-pairs-arithmetic",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11000001,
        .pattern = 0b00000001,
        .fine_grain = I8080InstrFineGrainRegPairsArith,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchRegPairsArith,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchRegPairsExchange(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080CycleCount     cycles;
    
    switch ( instr ) {
    
        case 0b11101011: {
            // XCHG
            I8080RegistersExchangeNamed8b(sys8080->rgstrs, H, D);
            I8080RegistersExchangeNamed8b(sys8080->rgstrs, L, E);
            DEBUG("Exchanged (H, L) <=> (D, E)");
            cycles = 4;
            break;
        }
        case 0b11100011: {
            // XTHL
            I8080Instr_t    to_L, to_H;
            
            to_L = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP);
            I8080MemWrite(sys8080->sysmem, sys8080->rgstrs.SP, sys8080->rgstrs.L);
            sys8080->rgstrs.L = to_L;
            
            to_H = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP + 1);
            I8080MemWrite(sys8080->sysmem, sys8080->rgstrs.SP + 1, sys8080->rgstrs.H);
            sys8080->rgstrs.H = to_H;
            DEBUG("Exchanged (H, L) with stack at ($%04hX, $%04hX)",
                sys8080->rgstrs.SP, sys8080->rgstrs.SP + 1);
            cycles = 18;
            break;
        }
        case 0b11111001: {
            // SPHL
            sys8080->rgstrs.SP = sys8080->rgstrs.HL;
            DEBUG("Set SP = HL=0x%04hX", sys8080->rgstrs.HL);
            cycles = 5;
            break;
        }
            
    
    }

    return cycles;
}

//

bool
I8080InstrFineGrainRegPairsExchange(
    I8080Instr_t    instr
)
{
    switch ( instr ) {
        case 0b11101011:
        case 0b11100011:
        case 0b11111001:
            return true;
    }
    return false;
}

const I8080InstrHandler_t I8080InstrHandlerRegPairsExchange = {
        .name = "register-pairs-exchange",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11100101,
        .pattern = 0b11100001,
        .fine_grain = I8080InstrFineGrainRegPairsExchange,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchRegPairsExchange,
        .next_handler = NULL
    };
