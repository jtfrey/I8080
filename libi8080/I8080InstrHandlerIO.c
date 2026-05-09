/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 I/O instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */

#include "I8080InstrHandlerIO.h"
#include "I8080DeviceIO.h"
#include "I8080System.h"
#include "I8080Logging.h"

I8080CycleCount
I8080InstrDispatchIO(
    I8080SystemPtr      sys8080,
    I8080Instr_t        instr
)
{
    I8080DevId          dev_id = I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++);
    
    if ( instr & 0b00001000 ) {
        // IN
        DEBUG("Reading 1 byte from device 0x%02hhX", dev_id);
        sys8080->rgstrs.A = I8080DevBusReadDevice(sys8080->devbus, dev_id);
        DEBUG("Received 0x%02hhX from device 0x%02hhX", sys8080->rgstrs.A, dev_id);
    } else {
        // OUT
        DEBUG("Writing 1 byte to device 0x%02X", dev_id);
        I8080DevBusWriteDevice(sys8080->devbus, dev_id, sys8080->rgstrs.A);
    }
    return 10;
}

//

const I8080InstrHandler_t I8080InstrHandlerIO = {
        .name = "i/o",
        .sub_type = kI8080InstrHandlerSubTypeOriginal,
        .mask = 0b11110111,
        .pattern = 0b11010011,
        .fine_grain = NULL,
        .action = kI8080InstrHandlerActionDispatch,
        .dispatch = I8080InstrDispatchIO,
        .next_handler = NULL
    };