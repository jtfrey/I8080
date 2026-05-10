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
#include <sys/stat.h>

#ifdef HAS_MMAP
#   include <sys/mman.h>
#endif

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
    if ( mem->callbacks.cleanup ) mem->callbacks.cleanup(mem, mem->context);
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
    I8080MemUnmappedContext_t  *unmapped = (I8080MemUnmappedContext_t*)context;
    
    if ( ! unmapped->unmapped_size || (*addr >= unmapped->base_address && (*addr - unmapped->base_address) < unmapped->unmapped_size) ) {
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
    I8080MemUnmappedContext_t  *unmapped = (I8080MemUnmappedContext_t*)context;
    
    if ( ! unmapped->unmapped_size || (*addr >= unmapped->base_address && (*addr - unmapped->base_address) < unmapped->unmapped_size) ) return true;
    if ( unmapped->next_callbacks && unmapped->next_callbacks->write ) {
        return unmapped->next_callbacks->write(mem, addr, byte, unmapped->next_context);
    }
    return false;
}

//

const I8080MemCallbacks __I8080MemUnmappedCallbacks = {
            .read = I8080MemUnmappedRead,
            .write = I8080MemUnmappedWrite,
            .cleanup = NULL
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
    I8080MemROMContext_t   *rom = (I8080MemROMContext_t*)context;
    I8080Addr_t             rom_addr = *addr - rom->rom_addr;
    
    if ( ! rom->rom_size || (*addr >= rom->rom_addr && rom_addr < rom->rom_size) ) {
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
    I8080MemROMContext_t   *rom = (I8080MemROMContext_t*)context;
    I8080Addr_t             rom_addr = *addr - rom->rom_addr;
    
    if ( ! rom->rom_size || (*addr >= rom->rom_addr && rom_addr < rom->rom_size) ) {
        return true;
    }
    if ( rom->next_callbacks && rom->next_callbacks->write ) {
        return rom->next_callbacks->write(mem, addr, byte, rom->next_context);
    }
    return false;
}

//

enum {
    kI8080MemROMOptsNeedMunmap = 0x80000000
};

static
void
I8080MemROMCleanup(
    I8080MemRef     mem,
    const void      *context
)
{
    I8080MemROMContext_t    *rom = (I8080MemROMContext_t*)context;
    
    if ( rom->rom_image && (rom->options & kI8080MemROMOptsNeedMunmap) ) {
        munmap(rom->rom_image, rom->rom_size ? rom->rom_size : 0x10000);
    }
    if ( rom->options & kI8080MemROMOptsDeallocateAtCleanup ) {
        free((void*)rom);
    }
}

//

I8080MemROMContextPtr
I8080MemROMWithBytes(
    I8080Addr_t     rom_addr, 
    const void      *bytes,
    I8080Addr_t     bytes_len,
    bool            should_copy
)
{
    I8080MemROMContext_t    *new_ROM = NULL;

    if ( bytes_len ) {
        if ( should_copy || ! bytes ) {
            new_ROM = (I8080MemROMContext_t*)calloc(1, sizeof(I8080MemROMContext_t) + 
                            bytes_len ? bytes_len : 0x10000);
        } else {
            new_ROM = (I8080MemROMContext_t*)calloc(1, sizeof(I8080MemROMContext_t));
        }
        if ( new_ROM ) {
            if ( should_copy || ! bytes ) {
                void        *dst = (void*)new_ROM + sizeof(I8080MemROMContext_t);
            
                if ( bytes ) memcpy(dst, bytes, bytes_len ? bytes_len : 0x10000);
                new_ROM->rom_image = (uint8_t*)dst;
            } else {
                new_ROM->rom_image = (uint8_t*)bytes;
            }
            new_ROM->rom_addr = rom_addr;
            new_ROM->rom_size = bytes_len;
        } else {
            ERROR("Unable to allocate %hd B ROM image (errno=%d)", bytes_len, errno);
        }
    }
    return new_ROM;
}

//

I8080MemROMContextPtr
I8080MemROMWithFilePtr(
    I8080Addr_t     rom_addr,
    FILE            *fptr,
    I8080Addr_t     bytes_len
)
{
    I8080MemROMContext_t    *new_ROM = NULL;
    
    if ( fptr && bytes_len ) {
        new_ROM = (I8080MemROMContext_t*)calloc(1, sizeof(I8080MemROMContext_t) + 
                        bytes_len ? bytes_len : 0x10000);
        if ( new_ROM ) {
            void        *dst = (void*)new_ROM + sizeof(I8080MemROMContext_t);
            
            fread(dst, 1, bytes_len ? bytes_len : 0x10000, fptr);
            
            new_ROM->rom_addr = rom_addr;
            new_ROM->rom_size = bytes_len;
            new_ROM->rom_image = (uint8_t*)dst;
            new_ROM->next_callbacks = NULL;
            new_ROM->next_context = NULL;
        } else {
            ERROR("Unable to allocate %hd B ROM image (errno=%d)", bytes_len, errno);
        }
    }
    return new_ROM;
}

//

I8080MemROMContextPtr
I8080MemROMWithFile(
    I8080Addr_t     rom_addr,
    const char      *filepath
)
{
    I8080MemROMContext_t    *new_ROM = NULL;
    struct stat             finfo;
    
    if ( stat(filepath, &finfo) == 0 ) {
        if ( finfo.st_size > 0 ) {
            FILE            *fptr = fopen(filepath, "rb");
            
            if ( fptr ) {
                I8080Addr_t bytes_len = (finfo.st_size < 0x10000) ? finfo.st_size : 0;
                new_ROM = I8080MemROMWithFilePtr(rom_addr, fptr, bytes_len);
            } else {
                ERROR("Unable to open file `%s` for ROM image read (errno=%d)", filepath, errno);
            }
        } else {
            ERROR("File `%s` is 0 B in size, will not create ROM image", filepath);
        }
    } else {
        ERROR("Could not stat `%s` file, will not create ROM image (errno=%d)", filepath, errno);
    }
    return new_ROM;        
}

//

I8080MemROMContextPtr
I8080MemROMWithByteRangeInFile(
    I8080Addr_t     rom_addr,
    const char      *filepath,
    off_t           offset,
    I8080Addr_t     length
)
{
    FILE                    *fptr = fopen(filepath, "rb");
    I8080MemROMContext_t    *new_ROM = NULL;
    
    if ( fptr ) {
        if ( fseeko(fptr, offset, SEEK_SET) == 0 ) {
            new_ROM = I8080MemROMWithFilePtr(rom_addr, fptr, length);
        } else {
            ERROR("Unable to seek to offset %lld in `%s` for ROM image read (errno=%d)", offset, filepath, errno);
        }
        fclose(fptr);
    } else {
        ERROR("Unable to open file `%s` for ROM image read (errno=%d)", filepath, errno);
    }
    return new_ROM;
}

//

#ifdef HAS_MMAP

I8080MemROMContextPtr
I8080MemROMWithMappedFile(
    I8080Addr_t     rom_addr,
    int             fd,
    off_t           offset,
    I8080Addr_t     length
)
{
    I8080MemROMContext_t    *new_ROM = NULL;
    struct stat             finfo;
    
    if ( fstat(fd, &finfo ) == 0 ) {
        off_t       bytes_in_file = finfo.st_size - offset;
        size_t      bytes_len = (bytes_in_file < 0x10000) ? bytes_in_file : 0x10000;
        
        if ( bytes_len ) {
            size_t  mmap_len;
            void    *segment;
            
            if ( length > bytes_len ) {
                mmap_len = bytes_len;
                length = (bytes_len == 0x10000) ? 0 : bytes_len;
            }
            else if ( length == 0 ) {
                mmap_len = bytes_len;
                if ( bytes_len < 0x10000 ) {
                    length = bytes_len;
                }
            }
            else {
                // length < bytes_len
                mmap_len = length;
            }
            
            segment = mmap(NULL, mmap_len, PROT_READ, MAP_FILE, fd, offset);
            if ( segment ) {
                new_ROM = (I8080MemROMContext_t*)calloc(1, sizeof(I8080MemROMContext_t));
                if ( new_ROM ) {
                    new_ROM->rom_addr = rom_addr;
                    new_ROM->rom_size = length;
                    new_ROM->rom_image = segment;
                } else {
                    ERROR("Unable to allocate %hd B ROM image (errno=%d)", mmap_len, errno);
                }
            } else {
                ERROR("Could not mmap fd %d, will not create ROM image (errno=%d)", fd, errno);
            }
        } else {
            ERROR("Fd %d is 0 B in size, will not create ROM image", fd);
        }
    } else {
        ERROR("Could not stat fd %d, will not create ROM image (errno=%d)", fd, errno);
    }
    return new_ROM;
}

#endif

//

const I8080MemCallbacks __I8080MemROMCallbacks = {
            .read = I8080MemROMRead,
            .write = I8080MemROMWrite,
            .cleanup = I8080MemROMCleanup
        };
const I8080MemCallbacks *I8080MemROMCallbacks = &__I8080MemROMCallbacks;
