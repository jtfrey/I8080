/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 Bank-Switched Memory expander.
 */
 
#ifndef __I8080BSM_H__
#define __I8080BSM_H__

#include "I8080Memory.h"
#include "I8080DeviceIO.h"
#include "I8080HostMemory.h"

/**
 * Bank-Switched Memory per-bank options
 */
typedef enum {
    kI8080BSMBankOptsIsWritable     = 1 << 0    /*!< the bank is RAM, not ROM */
} I8080BSMBankOpts_t;

/**
 * Opaque type of a Bank-Swapped Memory expander context
 */
typedef struct I8080BSMContext * I8080BSMContextPtr;

/**
 * Create a new Bank-Swapped Memory expander context
 * A sequence of \ref I8080HostMemory object and options terminated by a
 * NULL value is passed to this function to create a I8080BSMContext.
 * @param page_count        page count the BSM should occupy in
 *                          emulator system memory
 * @param bank0             the first memory bank content
 * @param bank0_opts        options for the first memory bank
 * @return                  a newly-initialized I8080BSMContext or NULL
 *                          on error
 */
I8080BSMContextPtr I8080BSMContextCreate(I8080Addr_t page_count, I8080HostMemoryRef bank0, I8080BSMBankOpts_t bank0_opts, ...);

/**
 * Create a new Bank-Swapped Memory expander context
 * Arrays of \ref I8080HostMemory objects and options are passed to
 * this function to create a I8080BSMContext.
 * @param page_count        page count the BSM should occupy in
 *                          emulator system memory
 * @param nbanks            number of \ref I8080HostMemory objects and
 *                          options in the arrays
 * @param banks             the array of \ref I8080HostMemory objects
 * @param opts              the array of options
 * @return                  a newly-initialized I8080BSMContext or NULL
 *                          on error
 */
I8080BSMContextPtr I8080BSMContextCreateWithBanks(I8080Addr_t page_count, int nbanks, I8080HostMemoryRef *memory, I8080BSMBankOpts_t *opts);

/**
 * Get the byte-size of the BSM in system memory
 * @param bsm       the BSM to query
 * @return          returns the size of the region of emulator system
 *                  memory occupied by this BSM
 */
I8080Addr_t I8080BSMContextBankSize(I8080BSMContextPtr bsm);

/**
 * Get the number of banks in the BSM
 * @param bsm       the BSM to query
 * @return          returns the number of banks of memory embodied by
 *                  this BSM
 */
uint8_t I8080BSMContextBankCount(I8080BSMContextPtr bsm);

/**
 * Switch the in-scope bank of the BSM
 * Set \p bsm to have memory bank \p bank_idx selected for memory
 * operations that hit the mapper.
 * @param bsm       the BSM to alter
 * @param bank_idx  the bank to switch to be in-scope
 */
void I8080BSMContextBankSelect(I8080BSMContextPtr bsm, uint8_t bank_idx);

/**
 * Get a pointer to the bytes present in the selected BSM bank
 * @param bsm       the BSM to query
 * @return          returns a pointer to the in-scope memory buffer
 */
uint8_t* I8080BSMContextBankPtr(I8080BSMContextPtr bsm);

/**
 * Bank-Switched Memory expander memory mapper
 * Address of the callbacks that implement a region of memory that
 * is backed by one or more independent banks of external memory.
 * An I/O device is used to switch between the in-focus bank that
 * is accessed via the mapped region of system memory.
 */
extern const I8080MemMapperCallbacks *I8080MemBSMCallbacks;

/** 
 * I/O device definition for controlling the in-scope region
 * of a Bank-Switched Memory expander.  The byte read from
 * this device indicates the bank that is selected; a byte
 * written to this device attempts to switch the expander to
 * that bank number.  A pointer to an \ref I8080BSMContext_t
 * (shared by the device and the memory subsystems) is provided
 * at registration.
 *
 * The consumer should copy the contents of this definition to
 * their own \ref I8080Device_t data structure rather than registering
 * \p I8080DeviceBSMInOut itself on a bus.
 */
extern const I8080DevicePtr I8080DeviceBSMInOut;

#endif /* __I8080BSM_H__ */
