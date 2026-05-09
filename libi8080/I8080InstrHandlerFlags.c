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
 
#include "I8080InstrHandlerFlags.h"
#include "I8080System.h"
#include "I8080Logging.h"

I8080CycleCount
I8080InstrDispatchFlagsCY(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    if ( I8080InstrExt(instr, 3, 3) == 0 ) {
        // STC
        sys8080->rgstrs.CY = 0b1;
        DEBUG("Set carry bit");
    } else {
        // CMC
        sys8080->rgstrs.CY ^= 0b1;
        DEBUG("Complement of carry bit CY=%X", sys8080->rgstrs.CY);
    }
    return 4;
}

//

const I8080InstrHandler_t I8080InstrHandlerFlagsCY = {
        .name = "flags-CY",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11110111,
        .pattern = 0b00110111,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchFlagsCY,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchFlagsINTE(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    if ( I8080InstrExt(instr, 3, 3) == 0 ) {
        // EI
        sys8080->rgstrs.INTE = 0b1;
        DEBUG("Set interrupt enable");
    } else {
        // DI
        sys8080->rgstrs.INTE = 0b0;
        DEBUG("Reset interrupt enable");
    }
    return 4;
}

//

const I8080InstrHandler_t I8080InstrHandlerFlagsINTE = {
        .name = "flags-INTE",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11110111,
        .pattern = 0b11110011,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchFlagsINTE,
        .next_handler = NULL
    };
