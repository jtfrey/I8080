/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implementation of the I8080 memory API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */

#include "I8080Memory.h"
#include "I8080Logging.h"

const I8080MemCallbacks I8080MemEmptyCallbacks = {
                                .read = NULL,
                                .write = NULL
                            };

//

typedef struct I8080Mem {
    I8080MemCallbacks   callbacks;  /*!< the callbacks for external interfacing */
    const void          *context;   /*!< the context to pass to callbacks */
    I8080MemPage_t*     pages[256]; /*!< array of 256 pointers to pages of memory */
} I8080Mem_t;

//

I8080MemRef
I8080MemCreate(
    const I8080MemCallbacks *callbacks,
    const void              *context
)
{
    I8080Mem_t  *mem = (I8080Mem_t*)calloc(1, sizeof(I8080Mem_t));
    
    if ( mem ) {
        if ( callbacks ) mem->callbacks = *callbacks;
        if ( context ) mem->context = context;
        INFO("Created memory subsystem %p", mem);
    }
    return mem;
}

//

void
I8080MemDestroy(
    I8080MemRef mem
)
{
    int         i = 0;
    while ( i < 256 ) {
        if ( mem->pages[i] ) {
            free((void*)mem->pages[i]);
            INFO("Deallocated page $%02X of memory subsystem", i);
        }
        i++;
    }
    free((void*)mem);
    INFO("Destroyed memory subsystem %p", mem);
}

//

void
I8080MemReset(
    I8080MemRef mem
)
{
    int         i = 0;
    while ( i < 256 ) {
        if ( mem->pages[i] ) {
            free((void*)mem->pages[i]);
            mem->pages[i] = NULL;
            INFO("Deallocated page $%02X of memory subsystem", i);
        }
        i++;
    }
}

//

void
I8080MemCopyIn(
    I8080MemRef     mem,
    const void      *bytes,
    size_t          bytes_len,
    I8080Addr_t     origin
)
{
    uint8_t         *ptr = (uint8_t*)bytes;
    while ( bytes_len-- ) I8080MemWrite(mem, origin++, *ptr++);
}

//

void
I8080MemCopyOut(
    I8080MemRef     mem,
    I8080Addr_t     origin,
    const void      *bytes,
    size_t          bytes_len
)
{
    uint8_t         *ptr = (uint8_t*)bytes;
    while ( bytes_len-- ) *ptr++ = I8080MemRead(mem, origin++);
}

//

uint8_t
I8080MemRead(
    I8080MemRef mem,
    I8080Addr_t addr
)
{
    uint8_t     out_byte = 0x00;
    
    if ( ! mem->callbacks.read || ! mem->callbacks.read(mem, &addr, &out_byte, mem->context) ) {
        if ( mem->pages[addr >> 8] ) out_byte = mem->pages[addr >> 8]->bytes[addr & 0xFF];
    }
    return out_byte;
}

//

void
I8080MemWrite(
    I8080MemRef mem,
    I8080Addr_t addr,
    uint8_t     byte
)
{
    if ( ! mem->callbacks.write || ! mem->callbacks.write(mem, &addr, byte, mem->context) ) {
        if ( ! mem->pages[addr >> 8] ) {
            mem->pages[addr >> 8] = (I8080MemPage_t*)calloc(1, sizeof(I8080MemPage_t));
            if ( ! mem->pages[addr >> 8] ) {
                CRITICAL("Unable to allocate page %hX of system memory", addr >> 8);
                exit(ENOMEM);
            }
            INFO("Allocated new page $%02X of memory subsystem",  (addr >> 8));
        }
        mem->pages[addr >> 8]->bytes[addr & 0xFF] = byte;
    }
}

//

uint8_t
I8080MemWriteThrough(
    I8080MemRef mem,
    I8080Addr_t addr,
    uint8_t     byte
)
{
    I8080MemWrite(mem, addr, byte);
    return I8080MemRead(mem, addr);
}

//
////
//

static
bool
I8080MemUnmappedRead(
    I8080MemRef         mem,
    I8080Addr_t         *addr,
    uint8_t             *byte,
    const void          *context
)
{
    I8080MemUnmapped_t  *unmapped = (I8080MemUnmapped_t*)context;
    
    if ( *addr >= unmapped->base_address && (*addr - unmapped->base_address) < unmapped->unmapped_size ) {
        *byte = unmapped->unmapped_byte;
        return true;
    }
    if ( unmapped->next_callbacks && unmapped->next_callbacks->read ) {
        return unmapped->next_callbacks->read(mem, addr, byte, unmapped->next_context);
    }
    return false;
}

//

static
bool
I8080MemUnmappedWrite(
    I8080MemRef         mem,
    I8080Addr_t         *addr,
    uint8_t             byte,
    const void          *context
)
{
    I8080MemUnmapped_t  *unmapped = (I8080MemUnmapped_t*)context;
    
    if ( *addr >= unmapped->base_address && (*addr - unmapped->base_address) < unmapped->unmapped_size ) return true;
    if ( unmapped->next_callbacks && unmapped->next_callbacks->write ) {
        return unmapped->next_callbacks->write(mem, addr, byte, unmapped->next_context);
    }
    return false;
}

//

const I8080MemCallbacks __I8080MemUnmappedCallbacks = {
            .read = I8080MemUnmappedRead,
            .write = I8080MemUnmappedWrite
        };
const I8080MemCallbacks *I8080MemUnmappedCallbacks = &__I8080MemUnmappedCallbacks;

//
////
//

static
bool
I8080MemROMRead(
    I8080MemRef     mem,
    I8080Addr_t     *addr,
    uint8_t         *byte,
    const void      *context
)
{
    I8080MemROM_t   *rom = (I8080MemROM_t*)context;
    I8080Addr_t     rom_addr = *addr - rom->rom_addr;
    
    if ( *addr >= rom->rom_addr && rom_addr < rom->rom_size ) {
        *byte = rom->rom_image[rom_addr];
        return true;
    }
    if ( rom->next_callbacks && rom->next_callbacks->read ) {
        return rom->next_callbacks->read(mem, addr, byte, rom->next_context);
    }
    return false;
}

//

static
bool
I8080MemROMWrite(
    I8080MemRef     mem,
    I8080Addr_t     *addr,
    uint8_t         byte,
    const void      *context
)
{
    I8080MemROM_t   *rom = (I8080MemROM_t*)context;
    I8080Addr_t     rom_addr = *addr - rom->rom_addr;
    
    if ( *addr >= rom->rom_addr && rom_addr < rom->rom_size ) {
        return true;
    }
    if ( rom->next_callbacks && rom->next_callbacks->write ) {
        return rom->next_callbacks->write(mem, addr, byte, rom->next_context);
    }
    return false;
}

//

const I8080MemCallbacks __I8080MemROMCallbacks = {
            .read = I8080MemROMRead,
            .write = I8080MemROMWrite
        };
const I8080MemCallbacks *I8080MemROMCallbacks = &__I8080MemROMCallbacks;
