/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 memory API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080MEMORY_H__
#define __I8080MEMORY_H__

#include "I8080Config.h"
#include "I8080TextBuffer.h"

/**
 * Address in memory
 * The 8080 uses 16-bit addresses.
 */
typedef uint16_t I8080Addr_t;

/**
 * Parse an address from a string
 * The \p in_str is parsed to get a \ref I8080Addr_t.
 * @param in_str            the string (maybe) containing an
 *                          address
 * @param out_addr          set to the parsed address if successful
 * @return                  boolean true if an address was parsed
 */
static inline
bool
I8080AddrParse(
    const char      *in_str,
    I8080Addr_t     *out_addr,
    const char*     *out_endstr
)
{
    char            *endptr = NULL;
    int             base = 0;
    long            v;
    
    if ( *in_str == '$' ) in_str++, base=16;
    v = strtol(in_str, &endptr, base);
    if ( (endptr == in_str) || (v < 0) || (v > 0xFFFF) ) return false;
    *out_addr = (I8080Addr_t)v;
    if ( out_endstr) *out_endstr = (const char*)endptr;
    return true;
}

/**
 * Return the base address of the page containing this address
 * @param ADDR          an address
 * @return              the address of the page in which ADDR resides
 */
#define I8080AddrToBasePage(ADDR)   ((ADDR) & 0xFF00)

/**
 * A page of memory
 * Memory is split into pages that are 256 bytes in size.  The offset within
 * a page is thus addressable using an 8-bit unsigned integer.
 *
 * Pages are allocated only as needed.
 */
typedef struct {
    uint8_t             bytes[256]; /*!< a page is 256 bytes */
} I8080MemPage_t;

/*
 * This is a forward declaration for the sake of the specification of
 * the memory mapper API.
 */
typedef struct I8080Mem * I8080MemRef;

/**
 * A range of addresses in emulated memory space
 */
typedef struct {
    I8080Addr_t     base;       /*!< the address of the first byte */
    I8080Addr_t     length;     /*!< the length of the range */
} I8080AddrRange_t;

/**
 * Create an address range
 * Convenience that returns a \ref I8080AddrRange_t initialized with
 * the given \p base and \p length values.
 * @param base          base address
 * @param length        number of bytes in the range
 * @return              a corresponding \ref I8080AddrRange_t
 */
static inline
I8080AddrRange_t
I8080AddrRangeCreate(
    I8080Addr_t     base,
    I8080Addr_t     length
)
{
    I8080AddrRange_t    out_range = { .base = base, .length = length };
    return out_range;
}

/**
 * Create an address range from two addresses
 * Convenience that returns a \ref I8080AddrRange_t initialized with
 * the range corresponding to the \p start_addr and \p end_addr.
 * 
 * When end_addr is less than start_addr, it implies the range
 * wraps from $FFFF back around at $0000 and the length is calculated
 * accordingly.  When the two addresses are equal, the entire range of
 * memory $0000-$FFFF is implied and the length will be zero.  Otherwise,
 * the length is calculated from \p start_addr through and including
 * \p end_addr:  the single byte at $0000 cannot be created since
 * the \p start_addr=$0000 and \p end_addr=$0000 is the special case
 * cited above.  In that case, just use e.g. \ref I8080AddrRangeCreate()
 * to create the single-byte range.
 * @param base          base address
 * @param length        number of bytes in the range
 * @return              a corresponding \ref I8080AddrRange_t
 */
static inline
I8080AddrRange_t
I8080AddrRangeCreateWithBounds(
    I8080Addr_t     start_addr,
    I8080Addr_t     end_addr
)
{
    I8080AddrRange_t    out_range = {
            .base = start_addr,
            .length = (end_addr <= start_addr) ? (0x10000U - (start_addr - end_addr)) : (end_addr - start_addr + 1)
        };
    return out_range;
}

/**
 * Resolve an address range to pages occupied
 * @param RANGE         the address range
 * @return              the address range based at the address of
 *                      the page of the base address and the final
 *                      byte of the page containing the final byte
 */
static inline
I8080AddrRange_t
I8080AddrRangeToPages(
    I8080AddrRange_t    in_range
)
{
    I8080AddrRange_t    out_range = {
            .base = I8080AddrToBasePage(in_range.base),
            .length = (in_range.length + 255) / 256
        };
    return out_range;
}

/**
 * Is an address within the range of addresses?
 * @param range         the address range
 * @param addr          the address to test
 * @return              boolean true if \p addr is within \p range
 */
static inline
bool
I8080AddrRangeContains(
    I8080AddrRange_t    range,
    I8080Addr_t         addr
)
{
    int         shift_addr = addr - range.base;
    return (shift_addr >= 0 && shift_addr < range.length);
}

static inline
I8080Addr_t
I8080AddrRangeEndAddr(
    I8080AddrRange_t    range
)
{
    uint32_t        end_addr = range.base + range.length - 1;
    if ( end_addr >= 0x10000 ) end_addr -= 0x10000;
    return end_addr;
}

/**
 * Memory mapper callback that performs initialization
 * When the system is reset, the memory subsystem must also reset all its
 * mappers.  This callback handles that action.
 * @param mem           reference to the system memory being reset
 * @param range         the page range covered by the mapper
 * @param context       opaque pointer to consumer-provided data
 */
typedef bool (*I8080MemMapperResetCallback)(I8080MemRef mem, I8080AddrRange_t range, const void *context);

/**
 * Memory mapper callback that modifies a target address
 * The most natural application of this callback is in memory mirroring:  the page
 * at $2000 is also presented at $4000.  If a mirroring mapper is registered on
 * $4000-$40FF, this function can subtract $2000 from \p *addr and return that
 * value:  e.g. $4040 - $2000 = $2040.  The memory location $2040 will be
 * read/written as though it were $4040.
 *
 * A precondition to this function's being called is that the \p addr has been
 * verified to lie within its page range.
 * @param mem           reference to a system memory
 * @param range         the page range covered by the mapper
 * @param addr          the address being accessed
 * @param context       opaque pointer to consumer-provided data
 * @return              the altered (or same) address
 */
typedef I8080Addr_t (*I8080MemMapperRewriteAddrCallback)(I8080MemRef mem, I8080AddrRange_t range, I8080Addr_t addr, const void *context);

/**
 * Memory mapper callback that reacts to memory reads
 * If the target address is within the address range of this mapper, this
 * function will be called to possibly override access to the actual page.
 *
 * A precondition to this function's being called is that the \p addr has been
 * verified to lie within its page range.
 * @param mem           reference to a system memory
 * @param range         the page range covered by the mapper
 * @param addr          the address being read
 * @param byte          pointer where the byte should be transferred
 * @param context       opaque pointer to consumer-provided data
 * @return              return boolean true if this function fully-handled
 *                      the read and the value in *byte is the actual
 *                      value the emulator code should see; boolean false 
 *                      propagates to the next mapper or ultimately to
 *                      IO8080Mem itself to read from a RAM page.
 */
typedef bool (*I8080MemMapperReadCallback)(I8080MemRef mem, I8080AddrRange_t range, I8080Addr_t addr, uint8_t *byte, const void *context);

/**
 * Memory mapper callback that reacts to memory writes
 * If the target address is within the address range of this mapper, this
 * function will be called to possibly override access to the actual page.
 *
 * A precondition to this function's being called is that the \p addr has been
 * verified to lie within its page range.
 * @param mem           reference to a system memory
 * @param range         the page range covered by the mapper
 * @param addr          the address being written
 * @param byte          the byte to be written
 * @param context       opaque pointer to consumer-provided data
 * @return              return boolean true if this function fully-handled
 *                      the write; boolean false propagates to the next mapper
 *                      or ultimately to IO8080Mem itself to write to a RAM page.
 */
typedef bool (*I8080MemMapperWriteCallback)(I8080MemRef mem, I8080AddrRange_t range, I8080Addr_t addr, uint8_t byte, const void *context);

/**
 * Memory mapper callback that does cleanup
 * When a system memory object is destroyed, this function cleans up any
 * state or resources associated with this mapper.  That can include the
 * deallocation of a dynamically-allocated \p context.
 * @param mem           reference to the system memory being destroyed
 * @param range         the page range covered by the mapper
 * @param context       opaque pointer to consumer-provided data
 */
typedef void (*I8080MemMapperShutdownCallback)(I8080MemRef mem, I8080AddrRange_t range, const void *context);

/**
 * Callback functions associated with an I8080Mem memory mapper
 * An I8080Mem object can have zero or more memory mappers associated with it.
 * Mappers are situated in front of all access to the actual pages of RAM
 * associated with the I8080Mem object and can serve a variety of functions:
 *
 *     - unmapped segments:  block all read/write to a range of addresses
 *     - ROM images:  attach external data (e.g. from a file) at an address
 *       in the emulated system's memory, blocking writes
 *     - mirroring:  intercept requests for addresses in $XXXX-$YYYY and
 *       substitute an alternative address
 *
 * Each registered mapper can provide an opaque pointer to addition contextual
 * data that it requires; that pointer is passed to the callback functions.
 */
typedef struct {
    const char                          *mapper_name;   /*!< NULL or a C string with the name of 
                                                             the mapper */
    I8080MemMapperRewriteAddrCallback   rewrite_addr;   /*!< NULL or a pointer to a function that
                                                             modifies the target address */
    I8080MemMapperReadCallback          read;           /*!< NULL or a pointer to a function to 
                                                             intercept reads */
    I8080MemMapperWriteCallback         write;          /*!< NULL or a pointer to a function to
                                                             intercept reads */
    I8080MemMapperResetCallback         reset;          /*!< NULL or a pointer to a function to
                                                             handle system reset */
    I8080MemMapperShutdownCallback      shutdown;       /*!< NULL or a pointer to a function to
                                                             cleanup on destroy */
} I8080MemMapperCallbacks;

/**
 * Reference to a registered memory mapper.
 */
typedef struct {
    I8080AddrRange_t            addr_range; /*!< the page range the mapper covers */
    I8080MemMapperCallbacks     callbacks;  /*!< the callbacks associated with the mapper */
    const void                  *context;   /*!< the context associated with the mapper */
} I8080MemMapperRef_t;

/**
 * The I8080 system memory
 * The system has 64 KiB of addressable memory.  This data structure
 * represents that memory space as an array of pointers to dynamically-
 * allocated pages of memory.  When initially created, there are no
 * actual pages of memory available.  When writes are performed against
 * the memory space, pages will be allocated as needed.
 *
 * This scheme also prevents pages that are effectively "blocked" by
 * a memory mapper from wasting memory resources.
 *
 * Mappers can be associated with the instance under the constraint
 * that only one can be associated with a page.  When a read/write
 * happens, the following happens:
 *
 *     - <page#> = (<addr> & 0xFF00) >> 8
 *     - handled = false
 *     - if mappers.by_page[<page#>]:
 *         - if mappers.by_page[<page#>]->rewrite_addr:
 *             - <addr> = mappers.by_page[<page#>]->rewrite_addr(<addr>)
 *             - <page#> = (<addr> & 0xFF00) >> 8
 *         - if mappers.by_page[<page#>]->read/write
 *             - handled = mappers.by_page[<page#>]->read/write(<addr>, <byte>)
 *     - if not handled:
 *         - <page> => pages[<page#>]
 *         - <byte> <=> <page>[<addr> & 0x00FF]
 *
 * This structure is only the public-facing portion of an instance;
 * there are additional private fields documented in the source for
 * this API.  The \p mapper_refs and \p pages fields are made public
 * so that inline variants of the read/write functions can be used
 * that may improve performance vs. function calls.
 */
typedef struct I8080Mem {
    I8080MemMapperRef_t*    mapper_refs[256];   /*!< possible mapper associated with each page */
    I8080MemPage_t*         pages[256];         /*!< array of 256 pointers to pages of memory */
} I8080Mem_t;

/**
 * Create a new system memory object
 * The new memory space is created without any pages populated.
 * @return              NULL on error or a newly-initialized system
 *                      memory
 */
I8080MemRef I8080MemCreate(void);

/**
 * Destroy a system memory object
 * Deallocates all memory pages and the \p mem object itself.
 * @param mem           the system memory object to destroy
 */
void I8080MemDestroy(I8080MemRef mem);

/**
 * Register a memory mapper
 * The callbacks and context in \p mapper will be registered with
 * \p mem on the pages indicated by the addr_range field of
 * \p mapper.
 *
 * If a collision occurs between this registration and a previously-
 * registered mapper, an error will be logged and false is returned.
 * @param mem           the system memory object with which \p mapper
 *                      should be registered
 * @param mapper        the callbacks, context, and address range to
 *                      be registered; \p mem makes a copy of the
 *                      data structure, so \p mapper can be modifed
 *                      after calling this function without affecting
 *                      the mapper
 * @return              boolean true if successful, false otherwise
 */
bool I8080MemRegisterMapper(I8080MemRef mem, I8080MemMapperRef_t *mapper);

/**
 * Reset a system memory object
 * Restores \p mem to its starting state.
 * @param mem           the system memory object to reset
 */
void I8080MemReset(I8080MemRef mem);

/**
 * Write a summary of the system memory object to a FILE stream
 * Writes a summary of the current configurational state of \p mem
 * to the FILE \p stream.
 * @param stream        the stream to which to write
 * @param mem           the system memory object
 */
void I8080MemPrint(FILE *stream, I8080MemRef mem);

/**
 * Write a summary of the system memory object to a text buffer
 * Writes a summary of the current configurational state of \p mem
 * to the text buffer \p tbuff.
 * @param tbuff         the text buffer to which to write
 * @param mem           the system memory object
 */
void I8080MemWriteToTextBuffer(I8080TextBufferRef tbuff, I8080MemRef mem);

/**
 * Copy external bytes into the system memory object
 * The memory buffer of \p bytes_len bytes located at \p bytes
 * is copied into \p mem starting at address \p origin.
 *
 * If the \p bytes_len and the location \p origin overflows the 64 KiB
 * memory addressing space, the copy will wrap around.  E.g. a 0x1000
 * byte buffer starting at \p origin $FF00 will place its first 256
 * bytes at the end of the 64 KiB space and the remaining 0x0F00 bytes
 * starting at $0000.
 * @param mem           the system memory object
 * @param bytes         the memory buffer to copy into \p mem
 * @param bytes_len     the number of bytes in \p bytes
 * @param origin        the starting location in \p mem at which to
 *                      place copied bytes
 */
void I8080MemCopyIn(I8080MemRef mem, const void *bytes, size_t bytes_len, I8080Addr_t origin);

/**
 * Copy bytes from the system memory object to an external buffer
 * The byte sequence of \p bytes_len located at \p origin in \p mem
 * and starting at address \p origin is copied to \p bytes.
 *
 * If the \p bytes_len and the location \p origin overflows the 64 KiB
 * memory addressing space, the copy will wrap around.  E.g. a 0x1000
 * byte buffer starting at \p origin $FF00 will copy the first 256
 * bytes from the end of the 64 KiB address space to the start of \p bytes
 * and the remaining 0x0F00 bytes will be copied starting at $0000.
 * @param mem           the system memory object
 * @param origin        the starting location in \p mem from which
 *                      bytes will be copied
 * @param bytes         the memory buffer to hold bytes from \p mem
 * @param bytes_len     the number of bytes to copy (\p mem must be
 *                      at least of this dimension)
 */
void I8080MemCopyOut(I8080MemRef mem, I8080Addr_t origin, const void *bytes, size_t bytes_len);

#ifdef I8080_MEMORY_ENABLE_INLINING

static inline
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

static inline
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

#else

/**
 * Read a byte from memory
 * Reads a byte from the system memory \p mem, applying any memory
 * mappers as necessary.
 * @param mem           the system memory object
 * @param addr          the address to be read
 * @return              the byte read from \p addr
 */
uint8_t I8080MemRead(I8080MemRef mem, I8080Addr_t addr);

/**
 * Write a byte to memory
 * Writes a \p byte to the system memory \p mem, applying any memory
 * mappers as necessary.
 * @param mem           the system memory object
 * @param addr          the address to be written
 * @return              the byte to write to \p addr
 */
void I8080MemWrite(I8080MemRef mem, I8080Addr_t addr, uint8_t byte);

#endif

/**
 * Write a byte to memory and read and return the result
 * Combines a \ref I8080MemWrite() followed by a \ref I8080MemRead()
 * of memory address \p addr.
 * @param mem           the system memory object
 * @param addr          the address to be written
 * @return              the byte read from \p addr after writing
 */
static inline
uint8_t
I8080MemWriteThrough(
    I8080MemRef     mem,
    I8080Addr_t     addr,
    uint8_t         byte
)
{
    I8080MemWrite(mem, addr, byte);
    return I8080MemRead(mem, addr);
}

/**
 * Unmapped segment memory mapper
 * Address of the callbacks that implement a region of memory that has no
 * backing hardware.  Writes are no-ops, reads return a static byte (passed
 * as a const void* context).
 */
extern const I8080MemMapperCallbacks *I8080MemUnmappedSegmentCallbacks;

/**
 * Type cast a byte to a memory mapper context
 * The only supporting data an unmapped segment needs is a single byte,
 * which is type cast to a void pointer.
 */
#define I8080MemUnmappedSegmentContext(B)   ((const void*)(B))


/**
 * Mirroring memory mapper
 * Address of the callbacks that implement a region of memory that
 * translates its addresses to an alternate region of memory.  E.g.
 * $4000-$40FF can be mapped to $2000-$20FF and either address will
 * reference the same page.
 */
extern const I8080MemMapperCallbacks *I8080MemMirroringCallbacks;

/**
 * Type cast a mirroring target address to a memory mapper context
 * The only supporting data a mirroring mapper needs is the target
 * address to which the segment is mapped.  This macro typecasts
 * a \ref I8080Addr_t to a void pointer.
 */
#define I8080MemMirroringContext(A) ((const void*)(A))



/**
 * ROM image memory mapper
 * Address of the callbacks that implement a region of memory that
 * is backed by storage external to the \ref I8080Mem_t instance's
 * memory pages and can only be read — writes are no-ops.
 */
extern const I8080MemMapperCallbacks *I8080MemROMCallbacks;

/**
 * ROM image memory mapper type
 * There are three types of ROM image available:
 *
 *     - a wrapper to an existing pointer and size
 *     - a buffer allocated as part of the context
 *       and filled with bytes
 *     - a pointer associated with a file that mmap()
 *       has mapped into the native memory system
 */
typedef enum {
    kI8080MemROMTypePtr = 0,    /*!< the ROM wraps an existing pointer of specified size */
    kI8080MemROMTypeAlloc,      /*!< the ROM includes a buffer holding the image bytes */
    kI8080MemROMTypeMMap        /*!< the ROM wraps a pointer returned by mmap() */
} I8080MemROMType_t;

/**
 * ROM image memory mapper context
 * An instance of this structure must be allocated and passed to
 * the \ref I8080Mem object when the ROM mapper is registered.  It
 * describes the ROM image that will be mapped into the emulated
 * system memory space.
 */
typedef struct {
    I8080MemROMType_t   rom_type;   /*!< the type of ROM image this mapper represents */
    const char          *rom_name;  /*!< a name for the ROM image (e.g. name of the source file); this
                                         should be set by the caller and it is the caller's responsibility
                                         to handle eventual disposal of the string */
    size_t              rom_size;   /*!< the actual size of the ROM image */
    I8080Addr_t         page_count; /*!< the number of pages the image nominally occupies in the
                                         emulated system's memory space */
    const uint8_t       *rom_bytes; /*!< pointer to the bytes comprising the ROM image */
} I8080MemROMContext_t;

/**
 * Type of a pointer to a ROM image mapper context
 */
typedef I8080MemROMContext_t * I8080MemROMContextPtr;

/**
 * Allocate a ROM image mapper context wrapping bytes in memory
 * If \p should_copy is false, then the \p bytes_len bytes at \p bytes is simply
 * wrapped by a \ref I8080MemROMContext_t; if \p should_copy is true, then the
 * \ref I8080MemROMContext_t is allocated with capacity to hold a copy of \p bytes.
 *
 * If \p bytes is NULL then a buffer of \p bytes_len is allocated and the \p rom_image
 * field in the returned \p I8080MemROMContextPtr can be modified by the caller.
 * @param bytes         pointer to the bytes comprising the ROM; if NULL, a buffer
 *                      will be created and zeroed and the caller can modify it as
 *                      desired
 * @param bytes_len     number of bytes in the ROM image, capped at 64 KiB
 * @param should_copy   if false then the returned \ref I8080MemROMContextPtr merely wraps
 *                      the \p bytes pointer -- which means \p bytes cannot be free'd
 *                      by the caller while the \p I8080MemROMContextPtr is in-use!
 * @param page_count    the nominal number of pages the ROM image should occupy; if
 *                      zero, then \p bytes_len is used to find the nearest page
 *                      boundary; if non-zero, the minimum size between \p bytes_len and
 *                      \p page_count dictates how large the segment will be
 * @return              pointer to the allocated and initialized I8080MemROMContext_t,
 *                      NULL on error
 */
I8080MemROMContextPtr I8080MemROMContextWithBytes(const void *bytes, size_t bytes_len, bool should_copy, I8080Addr_t page_count);

/**
 * Allocate a ROM image mapper context with bytes from an open file
 * Create a \ref I8080MemROMContext_t that wraps at most \p bytes_len bytes read from the
 * open FILE, \p fptr.  A memory buffer is created as part of the \ref I8080MemROMContext_t
 * to hold the bytes.
 * @param fptr          the file from which the bytes will be read
 * @param bytes_len     number of bytes in the ROM image; zero (0) implies the full contents
 *                      of the file OR at most 64 KiB
 * @param page_count    the nominal number of pages the ROM image should occupy;  passing a
 *                      value of zero will round-up \p bytes_len to the nearest page boundary;
 *                      if non-zero, the minimum size between \p bytes_len and \p page_count
 *                      dictates how large the segment will be (if \p bytes_len was zero, the
 *                      size from the file metadata will be used for the determination)
 * @return              pointer to the allocated and initialized I8080MemROMContext_t, NULL
 *                      on error
 */
I8080MemROMContextPtr I8080MemROMContextWithFilePtr(FILE *fptr, size_t bytes_len, I8080Addr_t page_count);

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
I8080MemROMContextPtr I8080MemROMContextWithFile(const char *filepath, I8080Addr_t page_count);

/**
 * Allocate a ROM image mapper context with a range of bytes from a file
 * Similar to \ref I8080MemROMWithFile(), but rather than reading bytes from
 * the start of the file the read can be repositioned using the \p offset
 * argument.  At most 64 KiB will be read.
 * @param filepath      the file from which the ROM bytes should be read
 * @param offset        offset at which the ROM should be read from the
 *                      file
 * @param length        number of bytes to read from the file; zero (0) implies the full
 *                      contents of the file OR at most 64 KiB
 * @param page_count    the nominal number of pages the ROM image should occupy;  passing a
 *                      value of zero will round-up \p length to the nearest page boundary;
 *                      if non-zero, the minimum size between \p length and \p page_count
 *                      dictates how large the segment will be (if \p length was zero, the
 *                      size from the file metadata will be used for the determination)
 * @return              pointer to the allocated and initialized I8080MemROMContext_t, NULL
 *                      on error
 */
I8080MemROMContextPtr I8080MemROMContextWithByteRangeInFile(const char *filepath, off_t src_offset, size_t src_length, I8080Addr_t page_count);

/**
 * Allocate a ROM image mapper context by mapping a file into virtual memory
 * Bytes in the file associated with file descriptor \p fd are mapped into the
 * native system's virtual address space.  The byte range (up to 64 KiB) starting
 * at \p offset in the file is in-scope.
 * @param src_fd        the file descriptor that will be passed to mmap()
 * @param offset        offset at which the file should be mmap'ed
 * @param length        number of bytes to mmap from the file; zero (0) implies the full
 *                      contents of the file OR at most 64 KiB
 * @param page_count    the nominal number of pages the ROM image should occupy;  passing a
 *                      value of zero will round-up \p length to the nearest page boundary;
 *                      if non-zero, the minimum size between \p length and \p page_count
 *                      dictates how large the segment will be (if \p length was zero, the
 *                      size from the file metadata will be used for the determination)
 * @return              pointer to the allocated and initialized I8080MemROMContext_t,
 *                      NULL on error
 */
I8080MemROMContextPtr I8080MemROMContextWithMappedFile(int src_fd, off_t src_offset, size_t src_length, I8080Addr_t page_count);

#endif /* __I8080MEMORY_H__ */
