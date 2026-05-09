/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 miscellaneous instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#include "I8080InstrHandlerMisc.h"
#include "I8080System.h"
#include "I8080Logging.h"

I8080CycleCount
I8080InstrDispatchMiscHALT(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{    
    DEBUG("HALT instruction encountered, clearing running state on system");
    sys8080->state &= ~kI8080SystemStateRunning;
    return 7;
}

//

const I8080InstrHandler_t I8080InstrHandlerMiscHALT = {
        .name = "misc-HALT",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11111111,
        .pattern = 0b01110110,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchMiscHALT,
        .next_handler = NULL
    };

//

I8080CycleCount
I8080InstrDispatchMiscNOP(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    DEBUG("No-op");
    return 4;
}

//

const I8080InstrHandler_t I8080InstrHandlerMiscNOP = {
        .name = "misc-NOP",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11111111,
        .pattern = 0b00000000,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchMiscNOP,
        .next_handler = NULL
    };