/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 Bank-Switched Memory expander.
 */

#include "I8080BSM.h"
#include "I8080Logging.h"

//

typedef struct {
    I8080BSMBankOpts_t  opts;
    I8080HostMemoryRef  memory;
} I8080BSMBank_t;

typedef struct I8080BSMContext {
    I8080Addr_t         bank_size;
    bool                is_single_image;
    uint8_t             bank_count;
    
    uint8_t             bank_select;
    uint8_t             *bank_ptr;
    size_t              bank_len;
    bool                is_writable;
    
    I8080BSMBank_t      banks[];
} I8080BSMContext_t;

//

I8080BSMContextPtr
I8080BSMContextCreate(
    I8080Addr_t         page_count,
    I8080HostMemoryRef  bank0,
    I8080BSMBankOpts_t  bank0_opts,
    ...
)
{
    va_list             argv;
    int                 nbanks = 0;
    I8080HostMemoryRef  bankN;
    I8080BSMContext_t   *new_context = NULL;
    
    if ( ! bank0 ) return NULL;
    va_start(argv, bank0_opts);
    do {
        nbanks++;
        bankN = va_arg(argv, I8080HostMemoryRef);
        if ( bankN ) va_arg(argv, I8080BSMBankOpts_t);
    } while ( bankN );
    va_end(argv);
    
    // Read to allocate the context:
    new_context = (I8080BSMContext_t*)calloc(1, sizeof(I8080BSMContext_t) + nbanks * sizeof(I8080BSMBank_t));
    if ( new_context ) {
        int         bank_idx;
        
        new_context->bank_size = 256 * page_count;
        if ( (new_context->is_single_image = (nbanks == 1)) ) {
            // Split the single memory segment into bank_size chunks:
            new_context->bank_count = (bank0->mem_size + new_context->bank_size - 1) / new_context->bank_size;
        } else {
            // Each memory segment is a bank:
            new_context->bank_count = nbanks;
        }
        
        va_start(argv, bank0_opts);
        bank_idx = 0;
        while ( bank_idx < nbanks ) {
            new_context->banks[bank_idx].opts = bank0_opts;
            new_context->banks[bank_idx].memory = I8080HostMemoryRetain(bank0);
            bank0 = va_arg(argv, I8080HostMemoryRef);
            bank0_opts = va_arg(argv, I8080BSMBankOpts_t);
            bank_idx++;
        }
        va_end(argv);
        
        I8080BSMContextBankSelect(new_context, 0);
    }
    return (I8080BSMContextPtr)new_context;
}

//

I8080BSMContextPtr
I8080BSMContextCreateWithBanks(
    I8080Addr_t         page_count,
    int                 nbanks,
    I8080HostMemoryRef  *memory,
    I8080BSMBankOpts_t  *opts
)
{
    I8080BSMContext_t   *new_context = NULL;

    new_context = (I8080BSMContext_t*)calloc(1, sizeof(I8080BSMContext_t) + nbanks * sizeof(I8080BSMBank_t));
    if ( new_context ) {
        int         bank_idx;
        
        new_context->bank_size = 256 * page_count;
        
        if ( (new_context->is_single_image = (nbanks == 1)) ) {
            // Split the single memory segment into bank_size chunks:
            new_context->bank_count = (memory[0]->mem_size + new_context->bank_size - 1) / new_context->bank_size;
        } else {
            // Each memory segment is a bank:
            new_context->bank_count = nbanks;
        }
        bank_idx = 0;
        while ( bank_idx < nbanks ) {
            new_context->banks[bank_idx].opts = *opts++;
            new_context->banks[bank_idx].memory = I8080HostMemoryRetain(*memory++);
            bank_idx++;
        }
        I8080BSMContextBankSelect(new_context, 0);
    }
    return (I8080BSMContextPtr)new_context;
}

//

I8080Addr_t
I8080BSMContextBankSize(
    I8080BSMContextPtr  bsm
)
{
    return bsm->bank_size;
}

//

uint8_t
I8080BSMContextBankCount(
    I8080BSMContextPtr  bsm
)
{
    return bsm->bank_count;
}

//

void
I8080BSMContextBankSelect(
    I8080BSMContextPtr  bsm,
    uint8_t             bank_idx
)
{
    if ( bank_idx < bsm->bank_count ) {
        size_t          nbytes;
        
        if ( bsm->is_single_image ) {
            nbytes = bsm->bank_size;
            if ( (bank_idx + 1) * nbytes > bsm->banks[0].memory->mem_size ) {
                nbytes = bsm->banks[0].memory->mem_size % bsm->bank_size;
            }
            bsm->bank_select = bank_idx;
            bsm->bank_ptr = (uint8_t*)bsm->banks[0].memory->mem_bytes + bank_idx * bsm->bank_size;
            bsm->bank_len = nbytes;
            bsm->is_writable = (bsm->banks[0].opts & kI8080BSMBankOptsIsWritable) == kI8080BSMBankOptsIsWritable;
        } else {
            nbytes = bsm->banks[bank_idx].memory->mem_size;
            bsm->bank_select = bank_idx;
            bsm->bank_ptr = (uint8_t*)bsm->banks[bank_idx].memory->mem_bytes;
            bsm->bank_len = ((nbytes > bsm->bank_size) ? bsm->bank_size : nbytes);
            bsm->is_writable = (bsm->banks[bank_idx].opts & kI8080BSMBankOptsIsWritable) == kI8080BSMBankOptsIsWritable;
        }
    }
}

//

uint8_t*
I8080BSMContextBankPtr(
    I8080BSMContextPtr  bsm
)
{
    return bsm->bank_ptr;
}

//
#pragma mark -
//

static
bool
I8080MemBSMRead(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             *byte,
    const void          *context
)
{
    I8080BSMContext_t   *BSM = (I8080BSMContext_t*)context;
    
    addr -= range.base;
    if ( addr < BSM->bank_size ) {
        *byte = (addr < BSM->bank_len) ? *(BSM->bank_ptr + addr) : 0x00;
        return true;
    }
    return false;
}

//

static
bool
I8080MemBSMWrite(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             byte,
    const void          *context
)
{
    I8080BSMContext_t   *BSM = (I8080BSMContext_t*)context;
    
    addr -= range.base;
    if ( addr < BSM->bank_size ) {
        if ( BSM->is_writable && (addr < BSM->bank_len) ) *(BSM->bank_ptr + addr) = byte;
        return true;
    }
    return false;
}

//

static
void
I8080MemBSMShutdown(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    const void          *context
)
{
    I8080BSMContext_t   *BSM = (I8080BSMContext_t*)context;
    
    if ( BSM->is_single_image ) {
        I8080HostMemoryRelease(BSM->banks[0].memory);
    } else {
        while ( BSM->bank_count-- ) I8080HostMemoryRelease(BSM->banks[BSM->bank_count].memory);
    }
    free((void*)BSM);
}

//

const I8080MemMapperCallbacks __I8080MemBSMCallbacks = {
            .mapper_name = "bank-switched-memory",
            .reset = NULL,
            .rewrite_addr = NULL,
            .read = I8080MemBSMRead,
            .write = I8080MemBSMWrite,
            .shutdown = I8080MemBSMShutdown
        };
const I8080MemMapperCallbacks *I8080MemBSMCallbacks = &__I8080MemBSMCallbacks;

//

static
uint8_t
I8080DeviceBSMInOutRead(
    I8080DevicePtr      dev,
    const void          *context
)
{
    I8080BSMContext_t   *BSM = (I8080BSMContext_t*)context;
    
    return BSM->bank_select;
}

//

static
void
I8080DeviceBSMInOutWrite(
    I8080DevicePtr  dev,
    uint8_t         byte,
    const void      *context
)
{
    I8080BSMContext_t   *BSM = (I8080BSMContext_t*)context;
    
    if ( (byte != BSM->bank_select) && (byte < BSM->bank_count) ) I8080BSMContextBankSelect(BSM, byte);
}

//

static
void
I8080DeviceBSMInOutReset(
    I8080DevicePtr  dev,
    const void      *context
)
{
    I8080BSMContextBankSelect((I8080BSMContext_t*)context, 0x00);
}

//

I8080Device_t __I8080DeviceBSMInOut = {
            .device_name = "bank-switched-memory",
            .device_mode = kI8080DeviceModeInputOutput,
            .input = {
                .read = I8080DeviceBSMInOutRead },
            .output = {
                .write = I8080DeviceBSMInOutWrite },
            .reset = I8080DeviceBSMInOutReset,
            .shutdown = NULL,
            .name = NULL
        };
const I8080DevicePtr I8080DeviceBSMInOut = &__I8080DeviceBSMInOut;
