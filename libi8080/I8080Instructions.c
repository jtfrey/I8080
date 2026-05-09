/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */

#include "I8080Instructions.h"
#include "I8080System.h"

const char* I8080InstrRegisterNames[] = { "B", "C", "D", "E", "H", "L", "F", "A" };

const char* I8080InstrRegisterPairNames[] = { "BC", "DE", "HL", "SP" };

//

I8080Instr_t
I8080InstrFetch(
    I8080SystemPtr  sys8080
)
{
    return I8080MemRead(sys8080->sysmem, sys8080->rgstrs.PC++);
}

//

static
I8080InstrDispatchCallback
__I8080InstrHandlerGetMatch(
    const I8080InstrHandler_t   *handlers,
    I8080Instr_t                instr,
    bool                        full_search,
    const char*                 *handler_name
)
{
    I8080InstrDispatchCallback  out_match = NULL;
    const char                  *matched_name = NULL;
    bool                        is_done = false;
    
    while ( ! is_done && handlers ) {
        const I8080InstrHandler_t   *chain = handlers;
        
        if ( handlers->sub_type != kI8080InstrHandlerSubTypeOriginal ) chain = handlers->reference;
        
        if ( (instr & chain->mask) == chain->pattern ) {
            if ( ! chain->fine_grain || chain->fine_grain(instr) ) {
                switch ( chain->action ) {
                    case kI8080InstrHandlerActionDispatch: {
                        if ( ! out_match ) {
                            out_match = chain->dispatch;
                            matched_name = chain->name;
                            is_done = ! full_search;
                        } else {
                            fprintf(stderr, "ERROR:  multiple matches for instruction %02hhX ('%s' vs. '%s')!\n", instr, matched_name, chain->name);
                        }
                        break;
                    }
                    case kI8080InstrHandlerActionSubHandler: {
                        I8080InstrDispatchCallback match = __I8080InstrHandlerGetMatch(chain->sub_handler, instr, full_search, &matched_name);
                        if ( ! out_match ) {
                            out_match = match;
                            is_done = ! full_search;
                        } else {
                            fprintf(stderr, "ERROR:  multiple matches for instruction %02hhX ('%s')!\n", instr, matched_name);
                        }
                        break;
                    }
                }
            }
        }
        handlers = handlers->next_handler;
    }
    if ( handler_name ) *handler_name = matched_name;
    return out_match;
}

//

I8080CycleCount
I8080InstrHandlerDispatch(
    const I8080InstrHandler_t   *handlers,
    I8080SystemPtr              sys8080,
    I8080Instr_t                instr
)
{
    I8080CycleCount             cycle_count = 0;
    I8080InstrDispatchCallback  dispatcher = __I8080InstrHandlerGetMatch(handlers, instr, false, NULL);
    
    if ( dispatcher ) cycle_count = dispatcher(sys8080, instr);
    return cycle_count;
}

//

void
I8080InstrTableInit(
    I8080InstrTable_t           *itbl,
    const I8080InstrHandler_t   *handlers
)
{
    I8080Instr_t                instr = 0;
    
    // Zero-out the whole table:
    memset(itbl, 0, sizeof(*itbl));
    
    // Loop over all 256 bit patterns:
    do {
        const char              *handler_name = NULL;
        itbl->dispatchers[instr] = __I8080InstrHandlerGetMatch(handlers, instr, true, &handler_name);
        if ( itbl->dispatchers[instr] ) {
            itbl->handler_names[instr] = handler_name;
            itbl->n_instructions++;
        }
    } while ( ++instr > 0 );
}

//

#include "I8080InstrHandlerAccum.h"
#include "I8080InstrHandlerDataMovement.h"
#include "I8080InstrHandlerDirectAddr.h"
#include "I8080InstrHandlerFlags.h"
#include "I8080InstrHandlerIO.h"
#include "I8080InstrHandlerJump.h"
#include "I8080InstrHandlerRegPairs.h"
#include "I8080InstrHandlerSingleReg.h"
#include "I8080InstrHandlerMisc.h"

static I8080InstrHandler_t __I8080DefaultInstrSet_Restart = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerRestart,
        .next_handler = NULL };

static I8080InstrHandler_t __I8080DefaultInstrSet_IO = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerIO,
        .next_handler = &__I8080DefaultInstrSet_Restart };

static I8080InstrHandler_t __I8080DefaultInstrSet_Return = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerReturn,
        .next_handler = &__I8080DefaultInstrSet_IO };

static I8080InstrHandler_t __I8080DefaultInstrSet_Call = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerCall,
        .next_handler = &__I8080DefaultInstrSet_Return };

static I8080InstrHandler_t __I8080DefaultInstrSet_Jump = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerJump,
        .next_handler = &__I8080DefaultInstrSet_Call };
        
static I8080InstrHandler_t __I8080DefaultInstrSet_DirectAddr = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerDirectAddr,
        .next_handler = &__I8080DefaultInstrSet_Jump };

static I8080InstrHandler_t __I8080DefaultInstrSet_RegPairsExchange = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerRegPairsExchange,
        .next_handler = &__I8080DefaultInstrSet_DirectAddr };

static I8080InstrHandler_t __I8080DefaultInstrSet_RegPairsStack = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerRegPairsStack,
        .next_handler = &__I8080DefaultInstrSet_RegPairsExchange };

static I8080InstrHandler_t __I8080DefaultInstrSet_RegPairsArith = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerRegPairsArith,
        .next_handler = &__I8080DefaultInstrSet_RegPairsStack };

static I8080InstrHandler_t __I8080DefaultInstrSet_AccumRotate = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerAccumRotate,
        .next_handler = &__I8080DefaultInstrSet_RegPairsArith };

static I8080InstrHandler_t __I8080DefaultInstrSet_AccumImmed = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerAccumImmed,
        .next_handler = &__I8080DefaultInstrSet_AccumRotate };

static I8080InstrHandler_t __I8080DefaultInstrSet_Accum = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerAccum,
        .next_handler = &__I8080DefaultInstrSet_AccumImmed };
        
static I8080InstrHandler_t __I8080DefaultInstrSet_SingleRegAccum = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerSingleRegAccum,
		.next_handler = &__I8080DefaultInstrSet_Accum };
		
static I8080InstrHandler_t __I8080DefaultInstrSet_SingleRegIncDec = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerSingleRegIncDec,
		.next_handler = &__I8080DefaultInstrSet_SingleRegAccum };
		
static I8080InstrHandler_t __I8080DefaultInstrSet_DataMovementLXI = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerDataMovementLXI,
        .next_handler = &__I8080DefaultInstrSet_SingleRegIncDec };
		
static I8080InstrHandler_t __I8080DefaultInstrSet_DataMovementMVI = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerDataMovementMVI,
        .next_handler = &__I8080DefaultInstrSet_DataMovementLXI };
		
static I8080InstrHandler_t __I8080DefaultInstrSet_DataMovementMov = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerDataMovementMov,
        .next_handler = &__I8080DefaultInstrSet_DataMovementMVI };

static I8080InstrHandler_t __I8080DefaultInstrSet_DataMovementAccum = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerDataMovementAccum,
        .next_handler = &__I8080DefaultInstrSet_DataMovementMov };
        
static I8080InstrHandler_t __I8080DefaultInstrSet_FlagsCY = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerFlagsCY,
        .next_handler = &__I8080DefaultInstrSet_DataMovementAccum };

static I8080InstrHandler_t __I8080DefaultInstrSet_FlagsINTE = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerFlagsINTE,
        .next_handler = &__I8080DefaultInstrSet_FlagsCY };

static I8080InstrHandler_t __I8080DefaultInstrSet_LoadPC = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerLoadPC,
        .next_handler = &__I8080DefaultInstrSet_FlagsINTE };

static I8080InstrHandler_t __I8080DefaultInstrSet_MiscHALT = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerMiscHALT,
        .next_handler = &__I8080DefaultInstrSet_LoadPC };

static I8080InstrHandler_t __I8080DefaultInstrSet_MiscNOP = {
		.sub_type = kI8080InstrHandlerSubTypeReference,
		.reference = &I8080InstrHandlerMiscNOP,
        .next_handler = &__I8080DefaultInstrSet_MiscHALT };


const I8080InstrHandler_t *I8080DefaultISA = &__I8080DefaultInstrSet_MiscNOP;
