/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 direct-addressing instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#include "I8080InstrHandlerDirectAddr.h"
#include "I8080System.h"

I8080CycleCount
I8080InstrDispatchDirectAddr(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080InstrDirectAddrOp_t    op = I8080InstrExtShift(instr, 3, 4);
    I8080Addr_t                 addr;
    I8080CycleCount             cycles;
    
    addr = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++) | (I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++) << 8);
    
    switch ( op ) {
        case kI8080InstrDirectAddrOpSTA: {
            I8080MemWrite(sys8080->sysmem, addr, sys8080->rgstrs.A);
            cycles = 13;
            break;
        }
        case kI8080InstrDirectAddrOpLDA: {
            sys8080->rgstrs.A = I8080MemRead(sys8080->sysmem, addr);
            cycles = 13;
            break;
        }
        
        case kI8080InstrDirectAddrOpSHLD: {
            I8080MemWrite(sys8080->sysmem, addr++, sys8080->rgstrs.L);
            I8080MemWrite(sys8080->sysmem, addr, sys8080->rgstrs.H);
            cycles = 16;
            break;
        }
        case kI8080InstrDirectAddrOpLHLD: {
            sys8080->rgstrs.L = I8080MemRead(sys8080->sysmem, addr++);
            sys8080->rgstrs.H = I8080MemRead(sys8080->sysmem, addr);
            cycles = 16;
            break;
        }
    }
    
    return cycles;
}

//

const I8080InstrHandler_t I8080InstrHandlerDirectAddr = {
        .name = "direct-address",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11100111,
        .pattern = 0b00100010,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchDirectAddr,
        .next_handler = NULL
    };
