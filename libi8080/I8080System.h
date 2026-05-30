/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 system API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080SYSTEM_H__
#define __I8080SYSTEM_H__

#include "I8080Registers.h"
#include "I8080Memory.h"
#include "I8080DeviceIO.h"
#include "I8080Instructions.h"
#include "I8080Timing.h"

#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
#   include <pthread.h>
#endif

/**
 * State of the system
 * Enumerated bitvector values that indicate the state the
 * emulated system is in.
 */
typedef enum {
    kI8080SystemStateOff        = 0b0,      /*!< power off, nothing happening */
    kI8080SystemStateOn         = 0b1,      /*!< power is on, may or may not be running code */
    kI8080SystemStateRunning    = 0b10,     /*!< a program has started running */
    kI8080SystemStateStall      = 0b100,    /*!< when a program is running but the CPU is being used for DMA */
    kI8080SystemStateBreak      = 0b1000,   /*!< execution was interrupted */
    kI8080SystemStateBadInstr   = 0b10000,  /*!< an invalid instruction was encountered */
    
    kI8080SystemStateError      = 0b11000   /*!< all error states */
} I8080SystemState_t;

/**
 * State == system is off?
 * Evaluates to true if \p STATE shows the system to be off.
 * @param STATE         value of type I8080SystemState_t
 */
#define I8080SystemIsOff(STATE)     ((STATE) == kI8080SystemStateOff)

/**
 * State == system is on?
 * Evaluates to true if \p STATE shows the system to be on.  Other
 * aspects of being on (running, error) are ignored.
 * @param STATE         value of type I8080SystemState_t
 */
#define I8080SystemIsOn(STATE)        (((STATE) & kI8080SystemStateOn) == kI8080SystemStateOn)

/**
 * State == system is ready?
 * Evaluates to true if \p STATE shows the system is on and not in
 * an error state.
 * @param STATE         value of type I8080SystemState_t
 */
#define I8080SystemIsReady(STATE)   ((((STATE) & kI8080SystemStateOn) == kI8080SystemStateOn) && \
                                     (((STATE) & kI8080SystemStateError) == 0))

/**
 * State == system is running?
 * Evaluates to true if \p STATE shows the system is on and currently
 * running a program
 * @param STATE         value of type I8080SystemState_t
 */
#define I8080SystemIsRunning(STATE) ((((STATE) & kI8080SystemStateRunning) == kI8080SystemStateRunning) && \
                                     (((STATE) & kI8080SystemStateError) == 0))

/**
 * Optional behaviors of the system
 * The monolithinc accelerated instruction handler has one major
 * caveat to it:  the parity and auxilliary carry condition bits
 * are NOT implemented.  The DAA opcode is present and will
 * convert a lone byte to BCD format, but nothing will carry
 * over to another byte.  The parity-even and -odd
 * jump/call/return opcodes are present but will never be
 * taken (since the parity bit never gets set).  If your 8080
 * code makes absolutely no use of BCD or the AC/P condition
 * bits, then you'll be fine -- and run much faster.
 */
typedef enum {
    kI8080SystemOpts2MHzClock       = 0b1,      /*!< constrain runtime to 2 MHz */
    kI8080SystemOptsAccelInstr      = 0b10,     /*!< use the monolithic accelerated instruction handler */
    
    kI8080SystemOptsAll             = 0b11      /*!< all options enabled */
} I8080SystemOpts_t;

/**
 * All subsystems of the 8080 machine
 * This data structure bundles-together all of the necessary
 * subsystems for an I8080 computer.  What's present here is
 * the most basic system:
 *
 *     - the register set
 *     - the default 8080 ISA
 *     - a 64 KiB system memory
 *     - a device bus with the standard "tty" device
 *       registered as device 0
 *
 * If there are additional devices necessary or a different
 * ISA should be loaded, request additional bytes when creating
 * the system.  The \p aux_data pointer can then be typecast
 * as a pointer to your own data structure of that specified
 * size.
 */
typedef struct I8080System {
    I8080SystemOpts_t       options;/*!< optional behaviors */
    I8080Registers_t        rgstrs; /*!< the register set */
    I8080MemRef             sysmem; /*!< the system memory */
    I8080DevBusRef          devbus; /*!< the device bus */
    I8080InstrTable_t       itbl;   /*!< the compiled instruction dispatch table */
    I8080Device_t           tty;    /*!< the standard "tty" device that will be registered on
                                         the device bus */
    I8080SystemState_t      state;  /*!< the state in which the system is currently operating */
    I8080Microseconds       last_cycle; /*!< timestamp of the last-run cycle */
    struct {
        I8080Microseconds   user_cpu;
        I8080Microseconds   sys_cpu;
        long                n_swaps, io_i, io_o;
    } rsrc_usage;                       /*!< filled-in after a call to I8080SystemRun() */
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
    struct {
        pthread_mutex_t     lock;       /*!< sync lock used by the interrupt API components */
        bool                is_raised;  /*!< an interrupt has been raised */
        I8080Instr_t        opcode;     /*!< the single-byte opcode provided by the interrupting device */
        uint64_t            count;      /*!< number of interrupts asserted */
    } interrupt;
#endif
    const void              *aux_data; /*!< pointer to the extra bytes requested as part of this
                                               instance */
} I8080System_t;

/**
 * Pointer to a system
 * Type of a pointer to a \ref I8080System_t.
 */
typedef I8080System_t * I8080SystemPtr;

/**
 * Create a new system
 * Creates and initializes a new system.  The returned \ref I8080SystemPtr
 * will have \p additional_bytes of storage allocated at the end of the
 * \ref I8080System_t fields, making it easy for additional fields to be
 * added.
 * @param options               optional behaviors
 * @param additional_bytes      allocate memory sized according to the
 *                              \ref I8080System_t data structure PLUS
 *                              this many bytes
 * @return                      the newly-initialized system or NULL
 */
I8080SystemPtr I8080SystemCreate(I8080SystemOpts_t options, size_t additional_bytes);

/**
 * Create a new system with modified TTY device behaviors
 * Creates and initializes a new system.  The returned \ref I8080SystemPtr
 * will have \p additional_bytes of storage allocated at the end of the
 * \ref I8080System_t fields, making it easy for additional fields to be
 * added.
 * @param options               optional behaviors
 * @param additional_bytes      allocate memory sized according to the
 *                              \ref I8080System_t data structure PLUS
 *                              this many bytes
 * @return                      the newly-initialized system or NULL
 */
I8080SystemPtr I8080SystemCreateWithTTYContext(I8080SystemOpts_t options, size_t additional_bytes, I8080DeviceTTYContext_t *tty_context);

/**
 * Destroy a system
 * Destroys the system memory and device bus owned by \p sys8080
 * then deallocates \p sys8080 itself.
 * @param sys8080       the system to destroy
 */
void I8080SystemDestroy(I8080SystemPtr sys8080);

/**
 * Reset a system
 * A system reset clears the memory, resets all devices on the
 * device bus, and zeroes-out the registers.  The state is set to
 * kI8080SystemStateOn, indicating power is on but no program is
 * yet running.
 * @param sys8080       the system to reset
 */
void I8080SystemReset(I8080SystemPtr sys8080);

/**
 * Turn a system on/off
 * Set the power state of the system.  If the new state \p is_on
 * matches the current state, false is returned.
 *
 * If the new state \p is_on=true, \ref I8080SystemReset() is
 * called to turn the system on.  Otherwise, all devices on the
 * device bus are shutdown and the state is set to
 * kI8080SystemStateOff.
 * @param sys8080       the system to alter
 * @param is_on         boolean true = turn the system on,
 *                      false = turn the system off
 * @return              boolean true if the state change was
 *                      successful
 */
bool I8080SystemSetPowerState(I8080SystemPtr sys8080, bool is_on);

/** 
 * Break out of running program
 * If the system is in the on and running state, force it into the
 * on but not running state.  The \ref kI8080SystemStateBreak
 * bit is set on the system state.
 * @param sys8080       the system to alter
 */
void I8080SystemBreak(I8080SystemPtr sys8080);

/**
 * Attempt to raise a hardware interrupt
 * On the 8080, a device that raises an interrupt provides the CPU with
 * a single one-byte opcode to be executed on the next dispatch cycle.
 *
 * Note that this function is always present in the API, but will only
 * be functional if I8080_SYSTEM_ENABLE_INTERRUPT_API was enabled at
 * build.
 * @param sys8080           the system to alter
 * @param interrupt_opcode  the single opcode to be executed
 */
void I8080SystemRaiseInterrupt(I8080SystemPtr sys8080, I8080Instr_t interrupt_opcode);

/**
 * Set the CPU to "stall" while DMA operations are happening.
 *
 * @param sys8080           the system to stall
 * @param should_stall      if true, set the stall state, otherwise
 *                          clear the stall state
 */
void I8080SystemSetStall(I8080SystemPtr sys8080, bool should_stall);

/**
 * Set the program counter
 * If \p sys8080 is in the on state, set the PC register to \p origin.
 * @param sys8080       the system to alter
 * @param origin        the new value of the PC
 * @return              boolean true if the PC was changed, false if
 *                      the system is not on
 */
bool I8080SystemSetPC(I8080SystemPtr sys8080, I8080Addr_t origin);

/**
 * Process a single instruction
 * If \p sys8080 is in the on state, fetch the next instruction
 * and process it.
 * @param sys8080       the system to step
 * @param cycles        if non-NULL, will be assinged the number
 *                      of cycles elapsed while executing the
 *                      instruction 
 * @return              boolean true if the system was on and the
 *                      instruction was a valid opcode, etc.;
 *                      false otherwise
 */
bool I8080SystemStep(I8080SystemPtr sys8080, I8080CycleCount *cycles);

/**
 * Run the emulator
 * Sets the PC to \p origin.  Then the \ref I8080SystemStep() function
 * is invoked repeatedly until a bad instruction is encountered or the
 * HLT instruction is encountered and the system transitions to non-running
 * state.
 * @param sys8080       the system to run
 * @param origin        the starting value of the PC
 */
void I8080SystemRun(I8080SystemPtr sys8080, I8080Addr_t origin);

/**
 * Write a summary of the system to a text buffer
 * Summarizes the state of \p sys8080 to the text stream \p tbuff.
 *
 * @param tbuff         the text buffer to which to write
 * @param sys8080       pointer to the system to summarize
 */
void I8080SystemWriteToTextBuffer(I8080TextBufferRef tbuff, I8080SystemPtr sys8080);

#endif /* __I8080SYSTEM_H__ */
