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
#include "I8080BasicALOps.h"
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
    I8080ALU_t              alu = I8080ALUCreate(sys8080->rgstrs.A, 0, sys8080->rgstrs.CY, false, true);
    I8080CycleCount         cycles = 4;
    
    switch ( r ) {
        case kI8080InstrRegisterB:
        case kI8080InstrRegisterC:
        case kI8080InstrRegisterD:
        case kI8080InstrRegisterE:
        case kI8080InstrRegisterH:
        case kI8080InstrRegisterL:
        case kI8080InstrRegisterA: {
            alu.OP2 = sys8080->rgstrs.R[I8080RgstrIdx(r)];
            DEBUG("Fetched operand 0x%02hhX from register %s", alu.OP2, I8080InstrRegisterNames[r]);
            break;
        }
        case kI8080InstrRegisterMem: {
            alu.OP2 = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.HL);
            DEBUG("Fetched operand 0x%02hhX from $%04hX", alu.OP2, sys8080->rgstrs.HL);
            cycles = 7;
            break;
        }
    }
    
    switch ( op ) {
        case kI8080InstrALUOpAddC:
            alu.WC = 0b1;
        case kI8080InstrALUOpAdd:
            I8080ALU2Add(&alu);
            DEBUG("Accumulator 0x%02hhX + 0x%02hhX = 0x%02hhX", alu.OP1, alu.OP2, alu.RES);
            sys8080->rgstrs.A = alu.RES;
            break;
        
        case kI8080InstrALUOpSubC:
            alu.WC = 0b1;
        case kI8080InstrALUOpSub:
            I8080ALU2Sub(&alu);
            DEBUG("Accumulator 0x%02hhX - 0x%02hhX = 0x%02hhX", alu.OP1, alu.OP2, alu.RES);
            sys8080->rgstrs.A = alu.RES;
            break;
            
        case kI8080InstrALUOpAnd:
            I8080ALU2And(&alu);
            DEBUG("Accumulator 0x%02hhX & 0x%02hhX = 0x%02hhX", alu.OP1, alu.OP2, alu.RES);
            sys8080->rgstrs.A = alu.RES;
            break;
            
        case kI8080InstrALUOpXor:
            I8080ALU2Xor(&alu);
            DEBUG("Accumulator 0x%02hhX | 0x%02hhX = 0x%02hhX", alu.OP1, alu.OP2, alu.RES);
            sys8080->rgstrs.A = alu.RES;
            break;
            
        case kI8080InstrALUOpOr:
            I8080ALU2Or(&alu);
            DEBUG("Accumulator 0x%02hhX ^ 0x%02hhX = 0x%02hhX", alu.OP1, alu.OP2, alu.RES);
            sys8080->rgstrs.A = alu.RES;
            break;
            
        case kI8080InstrALUOpCmp:
            I8080ALU2Sub(&alu);
            DEBUG("Accumulator (0x%02hhX == 0x%02hhX) = CY=%X, P=%X, AC=%X, Z=%X, S=%X",
                alu.OP1, -alu.OP2, alu.CY, alu.P, alu.AC, alu.Z, alu.S);
            break;
    }
    sys8080->rgstrs.Z = alu.Z;
    sys8080->rgstrs.S = alu.S;
    sys8080->rgstrs.AC = alu.AC;
    sys8080->rgstrs.P = alu.P;
    sys8080->rgstrs.CY = alu.CY;
    
    return cycles;
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
    I8080ALU_t              alu = I8080ALUCreate(sys8080->rgstrs.A, 0, sys8080->rgstrs.CY, false, true);
    
    switch ( op ) {
        case kI8080InstrRotateOpLeft:
            I8080ALU1RotL(&alu);
            DEBUG("Accumulator 0x%02hhX RLC = 0x%02hhX, CY=%X", alu.OP1, alu.RES, alu.CY);
            break;
        case kI8080InstrRotateOpRight:
            I8080ALU1RotR(&alu);
            DEBUG("Accumulator 0x%02hhX RRC = 0x%02hhX, CY=%X", alu.OP1, alu.RES, alu.CY);
            break;
        case kI8080InstrRotateOpLeftThruCarry:
            I8080ALU1RotCYL(&alu);
            DEBUG("Accumulator 0x%02hhX RAL = 0x%02hhX, CY=%X", alu.OP1, alu.RES, alu.CY);
            break;
        case kI8080InstrRotateOpRightThruCarry:
            I8080ALU1RotCYR(&alu);
            DEBUG("Accumulator 0x%02hhX RAR = 0x%02hhX, CY=%X", alu.OP1, alu.RES, alu.CY);
            break;
    }
    sys8080->rgstrs.A = alu.RES, sys8080->rgstrs.CY = alu.CY;
    return 4;
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
    I8080ALU_t              alu = I8080ALUCreate(sys8080->rgstrs.A,
                                        I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++),
                                        sys8080->rgstrs.CY, false, true);
                                        
    DEBUG("Fetched immediate operand 0x%02hhX from $%04hX", alu.OP2, sys8080->rgstrs.PC - 1);
    
    switch ( op ) {
        case kI8080InstrALUOpAddC:
            alu.WC = 0b1;
        case kI8080InstrALUOpAdd:
            I8080ALU2Add(&alu);
            DEBUG("Accumulator 0x%02hhX + 0x%02hhX = 0x%02hhX", alu.OP1, alu.OP2, alu.RES);
            sys8080->rgstrs.A = alu.RES;
            break;
        
        case kI8080InstrALUOpSubC:
            alu.WC = 0b1;
        case kI8080InstrALUOpSub:
            I8080ALU2Sub(&alu);
            DEBUG("Accumulator 0x%02hhX - 0x%02hhX = 0x%02hhX", alu.OP1, -alu.OP2, alu.RES);
            sys8080->rgstrs.A = alu.RES;
            break;
            
        case kI8080InstrALUOpAnd:
            I8080ALU2And(&alu);
            DEBUG("Accumulator 0x%02hhX & 0x%02hhX = 0x%02hhX", alu.OP1, alu.OP2, alu.RES);
            sys8080->rgstrs.A = alu.RES;
            break;
            
        case kI8080InstrALUOpXor:
            I8080ALU2Xor(&alu);
            DEBUG("Accumulator 0x%02hhX | 0x%02hhX = 0x%02hhX", alu.OP1, alu.OP2, alu.RES);
            sys8080->rgstrs.A = alu.RES;
            break;
            
        case kI8080InstrALUOpOr:
            I8080ALU2Or(&alu);
            DEBUG("Accumulator 0x%02hhX ^ 0x%02hhX = 0x%02hhX", alu.OP1, alu.OP2, alu.RES);
            sys8080->rgstrs.A = alu.RES;
            break;
            
        case kI8080InstrALUOpCmp:
            I8080ALU2Sub(&alu);
            DEBUG("Accumulator (0x%02hhX == 0x%02hhX) = CY=%X, P=%X, AC=%X, Z=%X, S=%X",
                alu.OP1, -alu.OP2, alu.CY, alu.P, alu.AC, alu.Z, alu.S);
            break;
    }
    sys8080->rgstrs.Z = alu.Z;
    sys8080->rgstrs.S = alu.S;
    sys8080->rgstrs.AC = alu.AC;
    sys8080->rgstrs.P = alu.P;
    sys8080->rgstrs.CY = alu.CY;
    
    return 7;
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
