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
    *byte = ( addr < ROM->rom_size ) ? ROM->rom_bytes[addr] : 0x00;
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
    
    switch ( ROM->rom_type ) {
        case kI8080MemROMTypePtr:
        case kI8080MemROMTypeAlloc:
            break;
        case kI8080MemROMTypeMMap:
#ifdef HAS_MMAP
            munmap((void*)ROM->rom_bytes, ROM->rom_size);
#endif
            break;
    }
    free((void*)ROM);
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

//
#pragma mark -
//

I8080MemROMContextPtr
I8080MemROMContextWithBytes(
    const void          *bytes,
    size_t              bytes_len,
    bool                should_copy,
    I8080Addr_t         page_count
)
{
    I8080MemROMContext_t    *new_ROM = NULL;
    size_t              alloc_len = bytes_len;
    
    if ( bytes_len == 0 && page_count == 0 ) {
        ERROR("Could not create ROM image with both zero bytes and zero pages!");
        return NULL;
    }
    if ( bytes_len == 0 ) {
        // page_count MUST be set, this is a prospective allocation
        // not backed by bytes (yet):
        alloc_len = 256 * page_count;
    }
    else {
        if ( page_count == 0 ) {
            page_count = (bytes_len + 255) / 256;
        }
        else if ( page_count * 256 < bytes_len ) {
            alloc_len = 256 * page_count;
        }
    }
    
    if ( ! should_copy && bytes ) {
        new_ROM = (I8080MemROMContext_t*)calloc(1, sizeof(I8080MemROMContext_t));
        if ( new_ROM ) {
            new_ROM->rom_type = kI8080MemROMTypePtr;
            new_ROM->rom_size = alloc_len;
            new_ROM->page_count = page_count;
            new_ROM->rom_bytes = bytes;
        } else {
            ERROR("Unable to allocate wrapped-pointer ROM image (errno=%d)", errno);
        }
    } else {
        size_t          obj_size = sizeof(I8080MemROMContext_t) + alloc_len;
        
        new_ROM = (I8080MemROMContext_t*)calloc(1, obj_size);
        if ( new_ROM ) {
            new_ROM->rom_type = kI8080MemROMTypeAlloc;
            new_ROM->rom_size = alloc_len;
            new_ROM->page_count = page_count;
            new_ROM->rom_bytes = (void*)new_ROM + sizeof(I8080MemROMContext_t);
            if ( bytes ) memcpy((void*)new_ROM->rom_bytes, bytes, alloc_len);
        } else {
            ERROR("Unable to allocate new buffered ROM image (errno=%d)", errno);
        }
    }
    return new_ROM;
    
}

//

I8080MemROMContextPtr
I8080MemROMContextWithFilePtr(
    FILE        *fptr,
    size_t      bytes_len,
    I8080Addr_t page_count
)
{
    I8080MemROMContext_t    *new_ROM = NULL;
    struct stat             finfo;
    off_t                   whence = ftello(fptr);
    
    if ( fstat(fileno(fptr), &finfo ) == 0 ) {
        off_t       bytes_in_file = finfo.st_size - whence;
        
        if ( bytes_in_file <= 0 ) {
            ERROR("Could not create mmap'ed ROM image, at the end of file");
            return NULL;
        }
        // Validate/fill-in the src_length and page_count:
        if ( bytes_len > bytes_in_file ) {
            WARNING("File containing mmap'ed ROM image is of size %1$lld, requested %2$lld; using %1$lld", bytes_in_file, bytes_len);
            bytes_len = bytes_in_file;
        }
        if ( bytes_len == 0 ) {
            if ( page_count > 0 ) {
                if ( 256 * page_count < bytes_in_file ) {
                    bytes_len = 256 * page_count;
                } else {
                    bytes_len = bytes_in_file;
                }
            } else {
                bytes_len = (bytes_in_file < 0x10000) ? bytes_in_file : 0x10000;
                page_count = (bytes_len + 255) / 256;
            }
        }
        else {
            if ( page_count > 0 ) {
                if ( 256 * page_count < bytes_len ) {
                    bytes_len = 256 * page_count;
                }
            } else {
                page_count = (bytes_len + 255) / 256;
            }
        }
        if ( bytes_len ) {
            size_t      obj_size = sizeof(I8080MemROMContext_t) + bytes_len;
            
            new_ROM = (I8080MemROMContext_t*)calloc(1, obj_size);
            if ( new_ROM ) {
                ssize_t     actual_len;
                
                new_ROM->rom_type = kI8080MemROMTypeAlloc;
                new_ROM->rom_size = bytes_len;
                new_ROM->page_count = page_count;
                new_ROM->rom_bytes = (void*)new_ROM + sizeof(I8080MemROMContext_t);
                actual_len = fread((void*)new_ROM->rom_bytes, 1, bytes_len, fptr);
                if ( actual_len != bytes_len ) {
                    ERROR("Failure to read full %lld B from ROM image", bytes_len);
                    free((void*)new_ROM);
                    new_ROM = NULL;
                }
            } else {
                ERROR("Unable to allocate %lld B ROM image (errno=%d)", bytes_len, errno);
            }
        } else {
            ERROR("Calculated ROM size is 0 B, will not create ROM image");
        }
    } else {
        ERROR("Could not stat source FILE, will not create ROM image (errno=%d)", errno);
    }
    return new_ROM;
}

/**
 * Allocate a ROM image mapper context with the contents of a file
 * Create a \ref I8080MemROMContext_t that wraps at most \p 64 KiB read from the file
 * at \p filepath.  A memory buffer is created as part of the \ref I8080MemROMContext_t
 * to hold the bytes.
 * @param filepath      the file from which the ROM bytes should be read
 * @param page_count    the nominal number of pages the ROM image should occupy;
 *                      passing a value of zero will read up to 64 KiB from the  file
 *                      and round-up the page count accordingly; passing a non-zero
 *                      value will limit the read to that many pages of data
 * @return              pointer to the allocated and initialized I8080MemROMContext_t, NULL
 *                      on error
 */
I8080MemROMContextPtr
I8080MemROMContextWithFile(
    const char  *filepath,
    I8080Addr_t page_count
)
{
    I8080MemROMContext_t    *new_ROM = NULL;
    struct stat             finfo;
    
    if ( stat(filepath, &finfo ) == 0 ) {
        off_t       bytes_in_file = finfo.st_size;
        
        if ( bytes_in_file <= 0 ) {
            ERROR("Could not create ROM image, file `%s` contains no data", filepath);
            return NULL;
        }
        if ( page_count == 0 ) {
            if ( bytes_in_file > 0x10000 ) bytes_in_file = 0x10000;
            page_count = (bytes_in_file + 255) / 256;
        } else {
            if ( page_count * 256 < bytes_in_file ) bytes_in_file = page_count * 256;
        }
        if ( bytes_in_file ) {
            FILE                *fptr = fopen(filepath, "rb");
            
            if ( fptr ) {
                new_ROM = I8080MemROMContextWithFilePtr(fptr, bytes_in_file, page_count);
                fclose(fptr);
            } else {
                ERROR("Unable to open file `%s` for ROM image read (errno=%d)", filepath, errno);
            }
        } else {
            ERROR("Calculated ROM size of `%s` is 0 B, will not create ROM image", filepath);
        }
    } else {
        ERROR("Could not stat `%s`, will not create ROM image (errno=%d)", filepath, errno);
    }
    return new_ROM;
}

//

I8080MemROMContextPtr
I8080MemROMContextWithByteRangeInFile(
    const char          *filepath,
    off_t               src_offset,
    size_t              src_length,
    I8080Addr_t         page_count
)
{
    I8080MemROMContext_t    *new_ROM = NULL;
    struct stat             finfo;
    
    if ( stat(filepath, &finfo ) == 0 ) {
        off_t       bytes_in_file = finfo.st_size - src_offset;
        
        if ( bytes_in_file <= 0 ) {
            ERROR("Could not create ROM image, offset %lld is beyond the end of file", src_offset);
            return NULL;
        }
        // Validate/fill-in the src_length and page_count:
        if ( src_length > bytes_in_file ) {
            WARNING("File containing ROM image is of size %1$lld, requested %2$lld; using %1$lld", bytes_in_file, src_length);
            src_length = bytes_in_file;
        }
        if ( src_length == 0 ) {
            if ( page_count > 0 ) {
                if ( 256 * page_count < bytes_in_file ) {
                    src_length = 256 * page_count;
                } else {
                    src_length = bytes_in_file;
                }
            } else {
                src_length = (bytes_in_file < 0x10000) ? bytes_in_file : 0x10000;
                page_count = (src_length + 255) / 256;
            }
        }
        else {
            if ( page_count > 0 ) {
                if ( 256 * page_count < src_length ) {
                    src_length = 256 * page_count;
                }
            } else {
                page_count = (src_length + 255) / 256;
            }
        }
        if ( src_length ) {
            FILE                *fptr = fopen(filepath, "rb");
            
            if ( fptr ) {
                new_ROM = I8080MemROMContextWithFilePtr(fptr, src_length, page_count);
                fclose(fptr);
            } else {
                ERROR("Unable to open file `%s` for ROM image read (errno=%d)", filepath, errno);
            }
        } else {
            ERROR("Calculated ROM size of `%s` is 0 B, will not create ROM image", filepath);
        }
    } else {
        ERROR("Could not stat `%s`, will not create ROM image (errno=%d)", filepath, errno);
    }
    return new_ROM;
}

//

I8080MemROMContextPtr
I8080MemROMContextWithMappedFile(
    int             src_fd,
    off_t           src_offset,
    size_t          src_length,
    I8080Addr_t     page_count
)
{
#ifndef HAS_MMAP

    ERROR("The mmap() ROM type is not supported on this system");
    return NULL;

#else
    I8080MemROMContext_t    *new_ROM = NULL;
    struct stat             finfo;
    
    if ( fstat(src_fd, &finfo ) == 0 ) {
        off_t       bytes_in_file = finfo.st_size - src_offset;
        
        if ( bytes_in_file <= 0 ) {
            ERROR("Could not create mmap'ed ROM image, offset %lld is beyond the end of file", src_offset);
            return NULL;
        }
        // Validate/fill-in the src_length and page_count:
        if ( src_length > bytes_in_file ) {
            WARNING("File containing mmap'ed ROM image is of size %1$lld, requested %2$lld; using %1$lld", bytes_in_file, src_length);
            src_length = bytes_in_file;
        }
        if ( src_length == 0 ) {
            if ( page_count > 0 ) {
                if ( 256 * page_count < bytes_in_file ) {
                    src_length = 256 * page_count;
                } else {
                    src_length = bytes_in_file;
                }
            } else {
                src_length = (bytes_in_file < 0x10000) ? bytes_in_file : 0x10000;
                page_count = (src_length + 255) / 256;
            }
        }
        else {
            if ( page_count > 0 ) {
                if ( 256 * page_count < src_length ) {
                    src_length = 256 * page_count;
                }
            } else {
                page_count = (src_length + 255) / 256;
            }
        }
        if ( src_length ) {
            void        *segment = mmap(NULL, src_length, PROT_READ, MAP_PRIVATE | MAP_FILE, src_fd, src_offset);
            
            if ( segment != MAP_FAILED ) {
                new_ROM = (I8080MemROMContext_t*)calloc(1, sizeof(I8080MemROMContext_t));
                if ( new_ROM ) {
                    new_ROM->rom_type = kI8080MemROMTypeMMap;
                    new_ROM->rom_size = src_length;
                    new_ROM->page_count = page_count;
                    new_ROM->rom_bytes = (uint8_t*)segment;
                } else {
                    ERROR("Unable to allocate %hd B mmap'ed ROM image (errno=%d)", src_length, errno);
                }
            } else {
                ERROR("Could not mmap fd %d, will not create ROM image (errno=%d)", src_fd, errno);
            }
        } else {
            ERROR("Calculated ROM size of fd %d is 0 B, will not create mmap'ed ROM image", src_fd);
        }
    } else {
        ERROR("Could not stat fd %d, will not create mmap'ed ROM image (errno=%d)", src_fd, errno);
    }
    return new_ROM;
#endif
}
