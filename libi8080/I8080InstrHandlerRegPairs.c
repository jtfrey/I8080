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
#include "I8080BasicALOps.h"
#include "I8080System.h"

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
                break;
            case kI8080InstrRegisterPairDE:
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.D);
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.E);
                break;
            case kI8080InstrRegisterPairHL:
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.H);
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.L);
                break;
            case kI8080InstrRegisterPairPSW:
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.A);
                I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, sys8080->rgstrs.F);
                break;
        }
        cycles = 11;
    } else {
        // POP
        switch ( pair ) {
            case kI8080InstrRegisterPairBC:
                sys8080->rgstrs.C = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                sys8080->rgstrs.C = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                break;
            case kI8080InstrRegisterPairDE:
                sys8080->rgstrs.E = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                sys8080->rgstrs.D = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                break;
            case kI8080InstrRegisterPairHL:
                sys8080->rgstrs.L = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                sys8080->rgstrs.H = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                break;
            case kI8080InstrRegisterPairPSW:
                sys8080->rgstrs.F = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
                sys8080->rgstrs.A = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++);
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
    I8080ALU_t                  alu;
    I8080CycleCount             cycles;
    
    switch ( instr & 0b00001110 ) {
        case 0b00001000: {
            // DAD
            alu = I8080ALUCreate(sys8080->rgstrs.L, 0, false, false, true);
            switch ( pair ) {
                case kI8080InstrRegisterPairBC:
                    alu.OP2 = sys8080->rgstrs.C;
                    break;
                case kI8080InstrRegisterPairDE:
                    alu.OP2 = sys8080->rgstrs.E;
                    break;
                case kI8080InstrRegisterPairHL:
                    alu.OP2 = sys8080->rgstrs.L;
                    break;
                case kI8080InstrRegisterPairSP:
                    alu.OP2 = sys8080->rgstrs.SP & 0x00FF;
                    break;
            }
            I8080ALU2Add(&alu);
            sys8080->rgstrs.L = alu.RES;
            alu.OP1 = sys8080->rgstrs.H, alu.WC = 0b1;
            switch ( pair ) {
                case kI8080InstrRegisterPairBC:
                    alu.OP2 = sys8080->rgstrs.B;
                    break;
                case kI8080InstrRegisterPairDE:
                    alu.OP2 = sys8080->rgstrs.D;
                    break;
                case kI8080InstrRegisterPairHL:
                    alu.OP2 = sys8080->rgstrs.H;
                    break;
                case kI8080InstrRegisterPairSP:
                    alu.OP2 = sys8080->rgstrs.SP >> 8;
                    break;
            }
            I8080ALU2Add(&alu);
            sys8080->rgstrs.H = alu.RES;
            sys8080->rgstrs.CY = alu.CY;
            cycles = 10;
            break;
        }
        case 0b00001010: {
            // INX
            alu = I8080ALUCreate(0, 0, false, false, true);
            switch ( pair ) {
                case kI8080InstrRegisterPairBC:
                    alu.OP1 = sys8080->rgstrs.C;
                    break;
                case kI8080InstrRegisterPairDE:
                    alu.OP1 = sys8080->rgstrs.E;
                    break;
                case kI8080InstrRegisterPairHL:
                    alu.OP1 = sys8080->rgstrs.L;
                    break;
                case kI8080InstrRegisterPairSP:
                    alu.OP1 = sys8080->rgstrs.SP & 0x00FF;
                    break;
            }
            I8080ALU1Inc(&alu);
            alu.SET = false;
            switch ( pair ) {
                case kI8080InstrRegisterPairBC:
                    sys8080->rgstrs.C = alu.RES;
                    if ( alu.CY ) {
                        alu.OP1 = sys8080->rgstrs.B;
                        I8080ALU1Inc(&alu);
                        sys8080->rgstrs.B = alu.RES;
                    }
                    break;
                case kI8080InstrRegisterPairDE:
                    sys8080->rgstrs.E = alu.RES;
                    if ( alu.CY ) {
                        alu.OP1 = sys8080->rgstrs.D;
                        I8080ALU1Inc(&alu);
                        sys8080->rgstrs.D = alu.RES;
                    }
                    break;
                case kI8080InstrRegisterPairHL:
                    sys8080->rgstrs.L = alu.RES;
                    if ( alu.CY ) {
                        alu.OP1 = sys8080->rgstrs.H;
                        I8080ALU1Inc(&alu);
                        sys8080->rgstrs.H = alu.RES;
                    }
                    break;
                case kI8080InstrRegisterPairSP:
                    sys8080->rgstrs.SP = (sys8080->rgstrs.SP & 0xFF00) | alu.RES;
                    if ( alu.CY ) {
                        alu.OP1 = sys8080->rgstrs.SP >> 8;
                        I8080ALU1Inc(&alu);
                        sys8080->rgstrs.SP = (alu.RES << 8) | (sys8080->rgstrs.SP & 0x00FF);
                    }
                    break;
            }
            cycles = 5;
            break;
        }
        case 0b00000010: {
            // DCX
            alu = I8080ALUCreate(0, 0, false, false, true);
            switch ( pair ) {
                case kI8080InstrRegisterPairBC:
                    alu.OP1 = sys8080->rgstrs.C;
                    break;
                case kI8080InstrRegisterPairDE:
                    alu.OP1 = sys8080->rgstrs.E;
                    break;
                case kI8080InstrRegisterPairHL:
                    alu.OP1 = sys8080->rgstrs.L;
                    break;
                case kI8080InstrRegisterPairSP:
                    alu.OP1 = sys8080->rgstrs.SP & 0x00FF;
                    break;
            }
            I8080ALU1Dec(&alu);
            alu.SET = false;
            switch ( pair ) {
                case kI8080InstrRegisterPairBC:
                    sys8080->rgstrs.C = alu.RES;
                    if ( alu.CY ) {
                        alu.OP1 = sys8080->rgstrs.B;
                        I8080ALU1Dec(&alu);
                        sys8080->rgstrs.B = alu.RES;
                    }
                    break;
                case kI8080InstrRegisterPairDE:
                    sys8080->rgstrs.E = alu.RES;
                    if ( alu.CY ) {
                        alu.OP1 = sys8080->rgstrs.D;
                        I8080ALU1Dec(&alu);
                        sys8080->rgstrs.D = alu.RES;
                    }
                    break;
                case kI8080InstrRegisterPairHL:
                    sys8080->rgstrs.L = alu.RES;
                    if ( alu.CY ) {
                        alu.OP1 = sys8080->rgstrs.H;
                        I8080ALU1Dec(&alu);
                        sys8080->rgstrs.H = alu.RES;
                    }
                    break;
                case kI8080InstrRegisterPairSP:
                    sys8080->rgstrs.SP = (sys8080->rgstrs.SP & 0xFF00) | alu.RES;
                    if ( alu.CY ) {
                        alu.OP1 = sys8080->rgstrs.SP >> 8;
                        I8080ALU1Dec(&alu);
                        sys8080->rgstrs.SP = (alu.RES << 8) | (sys8080->rgstrs.SP & 0x00FF);
                    }
                    break;
            }
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
            cycles = 18;
            break;
        }
        case 0b11111001: {
            // SPHL
            sys8080->rgstrs.SP = sys8080->rgstrs.H;
            sys8080->rgstrs.SP <<= 8;
            sys8080->rgstrs.SP |= sys8080->rgstrs.L;
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
