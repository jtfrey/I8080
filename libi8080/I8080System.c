/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 system API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */

#include "I8080System.h"
#include "I8080Logging.h"

static
I8080SystemPtr
I8080SystemAlloc(
    size_t          additional_bytes
)
{
    I8080System_t   *sys8080 = (I8080System_t*)calloc(1, additional_bytes + sizeof(I8080System_t));
    
    if ( sys8080 ) {
        if ( additional_bytes ) {
            sys8080->aux_data = (void*)sys8080 + sizeof(I8080System_t);
        } else {
            sys8080->aux_data = NULL;
        }
    }
    return sys8080;
}

//

I8080SystemPtr
I8080SystemCreate(
    I8080SystemOpts_t       options,
    size_t                  additional_bytes
)
{
    I8080System_t   *sys8080 = I8080SystemAlloc(additional_bytes);
    
    if ( sys8080 ) {
        sys8080->options = options & kI8080SystemOptsAll;
        sys8080->rgstrs = I8080Registers();
        sys8080->sysmem = I8080MemCreate(NULL, NULL);
        sys8080->devbus = I8080DevBusCreate();
        
        I8080InstrTableInit(&sys8080->itbl, I8080DefaultISA);
        
        sys8080->tty = *I8080DeviceTTY;
        I8080DevBusRegisterDevice(sys8080->devbus, 0, &sys8080->tty, NULL);
        
        sys8080->state = kI8080SystemStateOff;
        
        DEBUG("Created system %p", sys8080);
    }
    return sys8080;
}

//

I8080SystemPtr
I8080SystemCreateWithTTYContext(
    I8080SystemOpts_t       options,
    size_t                  additional_bytes,
    I8080DeviceTTYContext_t *tty_context
)
{
    I8080System_t   *sys8080 = I8080SystemAlloc(additional_bytes + sizeof(I8080DeviceTTYContext_t));
    
    if ( sys8080 ) {
        I8080DeviceTTYContext_t     *built_in_context;
        
        sys8080->options = options & kI8080SystemOptsAll;
        sys8080->rgstrs = I8080Registers();
        sys8080->sysmem = I8080MemCreate(NULL, NULL);
        sys8080->devbus = I8080DevBusCreate();
        
        I8080InstrTableInit(&sys8080->itbl, I8080DefaultISA);
        
        sys8080->tty = *I8080DeviceTTY;
        built_in_context = (I8080DeviceTTYContext_t*)sys8080->aux_data;
        sys8080->aux_data += sizeof(I8080DeviceTTYContext_t);
        *built_in_context = *tty_context;
        I8080DevBusRegisterDevice(sys8080->devbus, 0, &sys8080->tty, built_in_context);
        
        sys8080->state = kI8080SystemStateOff;
        
        DEBUG("Created system %p with TTY context", sys8080);
    }
    return sys8080;
}

//

void
I8080SystemDestroy(
    I8080SystemPtr  sys8080
)
{
    I8080DevBusDestroy(sys8080->devbus);
    I8080MemDestroy(sys8080->sysmem);
    free((void*)sys8080);
    DEBUG("Destroyed system %p", sys8080);
}

//

void
I8080SystemReset(
    I8080SystemPtr  sys8080
)
{
    sys8080->rgstrs = I8080Registers();
    I8080MemReset(sys8080->sysmem);
    I8080DevBusReset(sys8080->devbus);
    sys8080->state = kI8080SystemStateOn;
    INFO("System %p reset and ready", sys8080);
}

//

bool
I8080SystemSetPowerState(
    I8080SystemPtr  sys8080,
    bool            is_on
)
{
    bool            ok = false;
    
    if ( is_on ) {
        if ( ! (sys8080->state & kI8080SystemStateOn) ) {
            I8080SystemReset(sys8080);
            ok = true;
        }
    } else {
        if ( sys8080->state != kI8080SystemStateOff ) {
            I8080DevBusShutdown(sys8080->devbus);
            sys8080->state = kI8080SystemStateOff;
            INFO("System %p shutdown and off", sys8080);
            ok = true;
        }
    }
    return ok;
}

//

void
I8080SystemInterrupt(
    I8080SystemPtr  sys8080
)
{
    if ( I8080SystemIsReady(sys8080->state) ) {
        sys8080->state = (sys8080->state & ~kI8080SystemStateRunning) | kI8080SystemStateInterrupt;
    }
}

//

bool
I8080SystemSetPC(
    I8080SystemPtr  sys8080,
    I8080Addr_t     origin
)
{
    if ( I8080SystemIsReady(sys8080->state) ) {
        sys8080->rgstrs.PC = origin;
        DEBUG("Forcibly set the PC to $%04hX", origin);
        return true;
    }
    return false;
}

//

bool
I8080SystemStep(
    I8080SystemPtr  sys8080,
    I8080CycleCount *cycles
)
{
    bool            ok = false;
    
    if ( I8080SystemIsReady(sys8080->state) ) {
        I8080Instr_t    instr;
        I8080CycleCount elapsed = 0;
        
        if ( ! I8080SystemIsRunning(sys8080->state) ) {
            sys8080->state |= kI8080SystemStateRunning;
            sys8080->last_cycle = I8080MicrosecondsMakeNow();
            INFO("System transitioned to running state");
        }
        
        instr = I8080InstrFetch(sys8080);
        DEBUG("Fetched instruction: 0x%02hhX", instr);
        if ( sys8080->itbl.dispatchers[instr] ) {
            elapsed = sys8080->itbl.dispatchers[instr](sys8080, instr);
            sys8080->rgstrs.CYCLS += elapsed;
            
            if ( sys8080->options & kI8080SystemOpts2MHzClock ) {
                I8080Microseconds   now = I8080MicrosecondsMakeNow(),
                                    dt = (sys8080->last_cycle + (double)elapsed * 0.5) - now;
                
                if ( dt > 0.0 ) {
                    DEBUG("Sleeping for %.3f µs to fix clockspeed", dt);
                    I8080TimingSleep(dt);
                    now += dt;
                }
                sys8080->last_cycle = now;
            }
            ok = true;
        } else {
            // Stop running the program and indicate an invalid opcode was encountered:
            sys8080->state = kI8080SystemStateOn | kI8080SystemStateBadInstr;
            ERROR("An illegal instruction (0x%02hhX) was encountered", instr);
        }
        if ( cycles ) *cycles = elapsed;
    }
    return ok;
}

//

void
I8080SystemRun(
    I8080SystemPtr  sys8080,
    I8080Addr_t     origin
)
{
    if ( I8080SystemSetPC(sys8080, origin) ) {
        do {
            if ( ! I8080SystemStep(sys8080, NULL) ) break;
        } while ( I8080SystemIsRunning(sys8080->state) );
    }
}


