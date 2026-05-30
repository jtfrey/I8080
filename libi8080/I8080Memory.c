/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 memory API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */

#include "I8080Memory.h"
#include "I8080Logging.h"
#include <sys/stat.h>
#ifdef HAS_MMAP
#include <sys/mman.h>
#endif

/**
 * Linked list node holding a registered memory mapper
 * The set of memory mappers associated with a I8080Mem object is held
 * in a linked list comprised of these nodes.  The mappers must not
 * overlap in terms of the pages they span:  the \p addr_start and
 * \p map_size of each will be tested when registered to ensure
 * this is the case.
 */
typedef struct I8080MemMapper {
    struct I8080MemMapper   *link;          /*!< next mapper in chain */
    I8080MemMapperRef_t     mapper_data;
} I8080MemMapper_t;

/**
 * The private structure of the I8080Mem object
 * Additional non-public fields augment the public \ref I8080Mem_t.
 */
typedef struct {
    I8080Mem_t          public;         /*!< the public portion of the structure */
    I8080MemMapper_t    *list_head;     /*!< head of the list of registered mappers */
} I8080MemPrivate_t;

//

I8080MemRef
I8080MemCreate(void)
{
    I8080MemPrivate_t   *MEM = (I8080MemPrivate_t*)calloc(1, sizeof(I8080MemPrivate_t));
    
    if ( MEM ) {
        INFO("Created memory subsystem %p", MEM);
    }
    return (I8080MemRef)MEM;
}

//

void
I8080MemDestroy(
    I8080MemRef         mem
)
{
    I8080MemPrivate_t   *MEM = (I8080MemPrivate_t*)mem;
    I8080MemMapper_t    *mappers = MEM->list_head;
    I8080Addr_t         page;
    
    // Cleanup all mappers, deallocate their linked list nodes:
    while ( mappers ) {
        I8080MemMapper_t    *next = mappers->link;
        if ( mappers->mapper_data.callbacks.shutdown ) mappers->mapper_data.callbacks.shutdown(mem, mappers->mapper_data.addr_range, mappers->mapper_data.context);
        free((void*)mappers);
        mappers = next;
    }
    
    // Deallocate all pages:
    for ( page = 0; page < 256; page++ ) {
        if ( mem->pages[page] ) {
            free((void*)mem->pages[page]);
            INFO("Deallocated page $%02X of memory subsystem %p", page, mem);
        }
    }
    free((void*)mem);
    INFO("Destroyed memory subsystem %p", mem);
}

//

bool
I8080MemRegisterMapper(
    I8080MemRef         mem,
    I8080MemMapperRef_t *mapper
)
{
    I8080MemPrivate_t   *MEM = (I8080MemPrivate_t*)mem;
    I8080MemMapper_t    *new_node = malloc(sizeof(I8080MemMapper_t));
    
    if ( new_node ) {
        I8080Addr_t     addr = mapper->addr_range.base;
        int             len = mapper->addr_range.length;
        
        new_node->mapper_data = *mapper;
        new_node->link = MEM->list_head;
        MEM->list_head = new_node;
        
        // Now link to each page:
        while ( len > 0 ) {
            I8080Addr_t page = addr >> 8;
            
            if ( mem->mapper_refs[page] ) {
                ERROR("Collision with existing memory mapper on page $%02hX", page);
                free((void*)new_node);
                return false;
            }
            mem->mapper_refs[page] = &new_node->mapper_data;
            len -= 256, addr += 256;
        }
        return true;
    }
    return false;
}

//

enum {
    kI8080MemPrintEmptyPage,
    kI8080MemPrintAllocated,
    kI8080MemPrintMapper
};

#define PAGE_KIND(I) ((mem->mapper_refs[(I)]) ? kI8080MemPrintMapper : \
                        ((mem->pages[(I)]) ? kI8080MemPrintAllocated : \
                            kI8080MemPrintEmptyPage))

void
I8080MemPrint(
    FILE        *stream,
    I8080MemRef mem
)
{
    I8080MemPrivate_t   *MEM = (I8080MemPrivate_t*)mem;
    
    I8080Addr_t         page_s = 0, page_e = 1;
    int                 page_s_kind = PAGE_KIND(page_s),
                        page_e_kind;
    
    while ( true ) {
        bool            should_switch = true;
        
        if ( page_e < 256 ) {
            page_e_kind = PAGE_KIND(page_e);
            if ( page_e_kind == page_s_kind ) {
                should_switch = ! ((page_e_kind != kI8080MemPrintMapper) || (mem->mapper_refs[page_s] == mem->mapper_refs[page_e]));
            }
        }
        if ( should_switch ) {
            // Pages page_s through page_e - 1 can be summarized:
            switch ( page_s_kind ) {
                case kI8080MemPrintEmptyPage:
                    fprintf(stream, "I8080Mem[$%02hX00..$%02hXFF] : unused\n", page_s, page_e - 1);
                    break;
                case kI8080MemPrintAllocated:
                    fprintf(stream, "I8080Mem[$%02hX00..$%02hXFF] : allocated RAM\n", page_s, page_e - 1);
                    break;
                case kI8080MemPrintMapper: {
                    const char  *mapper_name = mem->mapper_refs[page_s]->callbacks.mapper_name;
                    fprintf(stream, "I8080Mem[$%02hX00..$%02hXFF] : mapper %p \"%s\"\n", page_s, page_e - 1,
                                        mem->mapper_refs[page_s], mapper_name ? mapper_name : "<unspecified>");
                    break;
                }
            }
            page_s = page_e;
            page_s_kind = page_e_kind;
        }
        if ( page_e == 256 ) break;
        page_e++;
    }
}

//

void
I8080MemWriteToTextBuffer(
    I8080TextBufferRef  tbuff,
    I8080MemRef         mem
)
{
    I8080MemPrivate_t   *MEM = (I8080MemPrivate_t*)mem;
    
    I8080Addr_t         page_s = 0, page_e = 1;
    int                 page_s_kind = PAGE_KIND(page_s),
                        page_e_kind;
    
    while ( true ) {
        bool            should_switch = true;
        
        if ( page_e < 256 ) {
            page_e_kind = PAGE_KIND(page_e);
            if ( page_e_kind == page_s_kind ) {
                should_switch = ! ((page_e_kind != kI8080MemPrintMapper) || (mem->mapper_refs[page_s] == mem->mapper_refs[page_e]));
            }
        }
        if ( should_switch ) {
            // Pages page_s through page_e - 1 can be summarized:
            switch ( page_s_kind ) {
                case kI8080MemPrintEmptyPage:
                    I8080TextBufferPrintf(tbuff, "I8080Mem[$%02hX00..$%02hXFF] : unused\n", page_s, page_e - 1);
                    break;
                case kI8080MemPrintAllocated:
                    I8080TextBufferPrintf(tbuff, "I8080Mem[$%02hX00..$%02hXFF] : allocated RAM\n", page_s, page_e - 1);
                    break;
                case kI8080MemPrintMapper: {
                    const char  *mapper_name = mem->mapper_refs[page_s]->callbacks.mapper_name;
                    I8080TextBufferPrintf(tbuff, "I8080Mem[$%02hX00..$%02hXFF] : mapper %p \"%s\"\n", page_s, page_e - 1,
                                        mem->mapper_refs[page_s], mapper_name ? mapper_name : "<unspecified>");
                    break;
                }
            }
            page_s = page_e;
            page_s_kind = page_e_kind;
        }
        if ( page_e == 256 ) break;
        page_e++;
    }
}

//

static inline
uint8_t
I8080MemCoreDumpTestZeroes(
    uint8_t     *ptr
)
{
    uint8_t     cumulative_or = *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    cumulative_or |= *ptr++;
    return (cumulative_or | *ptr) == 0x00;
}

void
I8080MemCoreDump(
    I8080MemRef             mem,
    I8080MemCoreDumpKind_t  core_kind,
    FILE                    *stream
)
{
    I8080MemPage_t          tmp_page;
    
    switch ( core_kind ) {
        case kI8080MemCoreDumpKindText: {
            I8080MemPrivate_t   *MEM = (I8080MemPrivate_t*)mem;
            
            I8080Addr_t         page_s = 0, page_e = 1;
            int                 page_s_kind = PAGE_KIND(page_s),
                                page_e_kind;
            
            while ( true ) {
                bool            should_switch = true;
                
                if ( page_e < 256 ) {
                    page_e_kind = PAGE_KIND(page_e);
                    if ( page_e_kind == page_s_kind ) {
                        should_switch = ! ((page_e_kind != kI8080MemPrintMapper) || (mem->mapper_refs[page_s] == mem->mapper_refs[page_e]));
                    }
                }
                if ( should_switch ) {
                    // Pages page_s through page_e - 1 can be summarized:
                    switch ( page_s_kind ) {
                        case kI8080MemPrintEmptyPage:
                            fprintf(stream, "$%02hX00: [E] 00 00 00 00 00 00 00 00    00 00 00 00 00 00 00 00\n"
                                            " *\n"
                                            "$%02hXF0: [E] 00 00 00 00 00 00 00 00    00 00 00 00 00 00 00 00\n",
                                    page_s, page_e - 1);
                            break;
                        case kI8080MemPrintMapper:
                            fprintf(stream, "$%02hX00: [M] 00 00 00 00 00 00 00 00    00 00 00 00 00 00 00 00\n"
                                            " *\n"
                                            "$%02hXF0: [M] 00 00 00 00 00 00 00 00    00 00 00 00 00 00 00 00\n",
                                    page_s, page_e - 1);
                            break;
                        case kI8080MemPrintAllocated: {
                            uint32_t        addr = (page_s << 8), end_addr = (page_e << 8);
                            uint32_t        zeroes_addr;
                            bool            is_zeroes = false;
                            uint8_t         *ptr;
                            
                            while ( addr < end_addr ) {
                                if ( (addr & 0x00FF) == 0 ) ptr = &mem->pages[page_s++]->bytes[0];
                                if ( I8080MemCoreDumpTestZeroes(ptr) ) {
                                    if ( ! is_zeroes ) {
                                        is_zeroes = true;
                                        zeroes_addr = addr;
                                    }
                                } else {
                                    if ( is_zeroes ) {
                                        // There was a zero range up until this 16-bytes:
                                        fprintf(stream, "$%04hX: [A] 00 00 00 00 00 00 00 00    00 00 00 00 00 00 00 00\n"
                                                        " *\n"
                                                        "$%04hX: [A] 00 00 00 00 00 00 00 00    00 00 00 00 00 00 00 00\n",
                                                zeroes_addr, addr - 16);
                                        is_zeroes = false;
                                    }
                                    fprintf(stream, "$%04hX: [A] %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX    %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX\n",
                                        addr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7],
                                        ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]);
                                }
                                ptr += 16, addr += 16;
                            }
                            if ( is_zeroes ) {
                                fprintf(stream, "$%04hX: [A] 00 00 00 00 00 00 00 00    00 00 00 00 00 00 00 00\n"
                                                " *\n"
                                                "$%04hX: [A] 00 00 00 00 00 00 00 00    00 00 00 00 00 00 00 00\n",
                                        zeroes_addr, addr - 16);
                            }
                            break;
                        }
                    }
                    page_s = page_e;
                    page_s_kind = page_e_kind;
                }
                if ( page_e == 256 ) break;
                page_e++;
            }
            break;
        }
        case kI8080MemCoreDumpKindBinary: {
            I8080Addr_t     addr = 0x0000;
            
            do {
                uint8_t     *p = &tmp_page.bytes[0];
                do {
                    *p++ = I8080MemRead(mem, addr++);
                } while ( addr & 0x00FF );
                fwrite(&tmp_page, sizeof(tmp_page), 1, stream);
            } while ( addr );
            break;
        }
    }
}

#undef PAGE_KIND

//

void
I8080MemReset(
    I8080MemRef         mem
)
{
    I8080MemPrivate_t   *MEM = (I8080MemPrivate_t*)mem;
    I8080MemMapper_t    *mappers = MEM->list_head;
    I8080Addr_t         page;
    
    // Deallocate all pages:
    for ( page = 0; page < 256; page++ ) {
        if ( mem->pages[page] ) {
            free((void*)mem->pages[page]);
            mem->pages[page] = NULL;
            INFO("Deallocated page $%02X of memory subsystem %p", page, mem);
        }
    }
    
    // Reset all mappers:
    while ( mappers ) {
        if ( mappers->mapper_data.callbacks.reset ) mappers->mapper_data.callbacks.reset(mem, mappers->mapper_data.addr_range, mappers->mapper_data.context);
        mappers = mappers->link;
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

#ifndef I8080_MEMORY_ENABLE_INLINING

uint8_t
I8080MemRead(
    I8080MemRef     mem,
    I8080Addr_t     addr
)
{
#define  MAPPER     mem->mapper_refs[page]

    I8080Addr_t     page = addr >> 8;
    
    if ( MAPPER && I8080AddrRangeContains(MAPPER->addr_range, addr)) {
        if ( MAPPER->callbacks.rewrite_addr ) {
            addr = MAPPER->callbacks.rewrite_addr(mem, MAPPER->addr_range, addr, MAPPER->context);
            page = addr >> 8;
        }
        if ( MAPPER && I8080AddrRangeContains(MAPPER->addr_range, addr) && MAPPER->callbacks.read ) {
            uint8_t     byte;
            if ( MAPPER->callbacks.read(mem, MAPPER->addr_range, addr, &byte, MAPPER->context) ) return byte;
        }
    }
    return mem->pages[page] ? mem->pages[page]->bytes[addr & 0xFF] : 0x00;

#undef MAPPER
}

//

void
I8080MemWrite(
    I8080MemRef     mem,
    I8080Addr_t     addr,
    uint8_t         byte
)
{
#define  MAPPER     mem->mapper_refs[page]

    I8080Addr_t     page = addr >> 8;
    
    if ( MAPPER && I8080AddrRangeContains(MAPPER->addr_range, addr) ) {
        if ( MAPPER->callbacks.rewrite_addr ) {
            addr = MAPPER->callbacks.rewrite_addr(mem, MAPPER->addr_range, addr, MAPPER->context);
            page = addr >> 8;
        }
        if ( MAPPER && I8080AddrRangeContains(MAPPER->addr_range, addr) && MAPPER->callbacks.write ) {
            if ( MAPPER->callbacks.write(mem, MAPPER->addr_range, addr, byte, MAPPER->context) ) return;
        }
    }
    if ( ! mem->pages[page] ) mem->pages[page] = (I8080MemPage_t*)calloc(1, sizeof(I8080MemPage_t));
    mem->pages[page]->bytes[addr & 0xFF] = byte;

#undef MAPPER
}

#endif

//
#pragma mark -
//

static
bool
I8080MemUnmappedSegmentRead(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             *byte,
    const void          *context
)
{
    *byte = (uint8_t)(uintptr_t)context;
    return true;
}

//

static
bool
I8080MemUnmappedSegmentWrite(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             byte,
    const void          *context
)
{
    return true;
}

//

const I8080MemMapperCallbacks __I8080MemUnmappedSegmentCallbacks = {
            .mapper_name = "unmapped-segment",
            .reset = NULL,
            .rewrite_addr = NULL,
            .read = I8080MemUnmappedSegmentRead,
            .write = I8080MemUnmappedSegmentWrite,
            .shutdown = NULL
        };
const I8080MemMapperCallbacks *I8080MemUnmappedSegmentCallbacks = &__I8080MemUnmappedSegmentCallbacks;

//
#pragma mark -
//

static
I8080Addr_t
I8080MemMirroringRewrite(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    const void          *context
)
{
    I8080Addr_t         target = (I8080Addr_t)(uintptr_t)context;
    
    return target + (addr - range.base);
}

//

const I8080MemMapperCallbacks __I8080MemMirroringCallbacks = {
            .mapper_name = "addr-mirror",
            .reset = NULL,
            .rewrite_addr = I8080MemMirroringRewrite,
            .read = NULL,
            .write = NULL,
            .shutdown = NULL
        };
const I8080MemMapperCallbacks *I8080MemMirroringCallbacks = &__I8080MemMirroringCallbacks;

//
#pragma mark -
//

static
bool
I8080MemROMRead(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             *byte,
    const void          *context
)
{
    I8080MemROMContext_t    *ROM = (I8080MemROMContext_t*)context;
    
    addr -= range.base;
    *byte = ( addr < ROM->rom_image->mem_size ) ? *(uint8_t*)(ROM->rom_image->mem_bytes + addr) : 0x00;
    return true;
}

//

static
bool
I8080MemROMWrite(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             byte,
    const void          *context
)
{
    return true;
}

//

static
void
I8080MemROMShutdown(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    const void          *context
)
{
    I8080MemROMContext_t    *ROM = (I8080MemROMContext_t*)context;
    I8080HostMemoryRelease(ROM->rom_image);
}

//

const I8080MemMapperCallbacks __I8080MemROMCallbacks = {
            .mapper_name = "ROM-image",
            .reset = NULL,
            .rewrite_addr = NULL,
            .read = I8080MemROMRead,
            .write = I8080MemROMWrite,
            .shutdown = I8080MemROMShutdown
        };
const I8080MemMapperCallbacks *I8080MemROMCallbacks = &__I8080MemROMCallbacks;
