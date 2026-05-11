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
 * A page of memory
 * Memory is split into pages that are 256 bytes in size.  The offset within
 * a page is thus addressable using an 8-bit unsigned integer.
 */
typedef struct {
    uint8_t         bytes[256]; /*!< a page is 256 bytes */
} I8080MemPage_t;

/**
 * The I8080 system memory
 * The system has 64 KiB of addressable memory.  This data structure
 * represents that memory space as an array of pointers to dynamically-
 * allocated pages of memory.  When initially created, there are no
 * actual pages of memory available.  When writes are performed against
 * the memory space, pages will be allocated as needed.
 */
typedef struct I8080Mem * I8080MemRef;

/**
 * External function that intercepts memory reads
 * The consumer code can provide a function that will be called in order
 * to allow reads against addresses to be intercepted and modified.
 * @param mem           reference to a system memory
 * @param addr          pointer to the address being read; the arbiter
 *                      can modify the address and return false to rewrite
 *                      where in RAM the byte is coming from (mirroring)
 * @param byte          pointer where the byte should be transferred
 * @param context       opaque pointer to consumer-provided data
 * @return              the function should return boolean true if it
 *                      handled the read and the IO8080Mem class should
 *                      not do anything; boolean false allows IO8080Mem
 *                      to handle the read itself
 */
typedef bool (*I8080MemReadArbiterCallback)(I8080MemRef mem, I8080Addr_t* addr, uint8_t *byte, const void *context);

/**
 * External function that intercepts memory writes
 * The consumer code can provide a function that will be called in order
 * to allow writes against addresses to be intercepted and modified.
 * @param mem           reference to a system memory
 * @param addr          pointer to the address being written; the arbiter
 *                      can modify the address and return false to rewrite
 *                      where in RAM the byte is going (mirroring)
 * @param byte          the byte to be transferred
 * @param context       opaque pointer to consumer-provided data
 * @return              the function should return boolean true if it
 *                      handled the write and the IO8080Mem class should
 *                      not do anything; boolean false allows IO8080Mem
 *                      to handle the write itself
 */
typedef bool (*I8080MemWriteArbiterCallback)(I8080MemRef mem, I8080Addr_t *addr, uint8_t byte, const void *context);

/**
 * External function that does cleanup
 * When a system memory object is destroyed, this function frees any
 * resources associated with the callbacks registered with the object.
 *
 * This can include deallocation of a dynamically-allocated \p
 * context.
 * @param mem           reference to the system memory being destroyed
 */
typedef void (*I8080MemCleanupCallback)(I8080MemRef mem, const void *context);

/**
 * External callback functions available to an IO8080Mem object
 * External callback functions on a system memory object allow greater
 * flexibility in how the memory behaves.  For example:
 *
 *     typedef struct {
 *         I8080Addr_t  rom_addr;
 *         I8080Addr_t  rom_size;
 *         uint8_t      *rom_image;
 *     } ROM_t;
 *
 *     bool ROMRead(I8080MemRef mem, I8080Addr_t addr, uint8_t *byte, const void *context)
 *     {
 *         ROM_t        *rom = (ROM_t*)context;
 *         I8080Addr_t    rom_addr = addr - rom->rom_addr;
 *         if ( addr >= rom->rom_addr && rom_addr < rom->rom_size ) {
 *             *byte = rom->rom_image[rom_addr];
 *             return true;
 *         }
 *         return false;
 *
 *     bool ROMWrite(I8080MemRef mem, I8080Addr_t addr, uint8_t byte, const void *context)
 *     {
 *         ROM_t        *rom = (ROM_t*)context;
 *         I8080Addr_t    rom_addr = addr - rom->rom_addr;
 *         return ( addr >= rom->rom_addr && rom_addr < rom->rom_size );
 *     }
 *
 * The address range and array of bytes present in ROM_t cannot be written to and
 * the \ref I8080Mem object will not create page mappings for pages that are
 * fully-contained by the ROM image.
 */
typedef struct {
    I8080MemReadArbiterCallback     read;           /*!< NULL or a pointer to a function to intercept reads */
    I8080MemWriteArbiterCallback    write;          /*!< NULL or a pointer to a function to intercept reads */
    I8080MemCleanupCallback         cleanup;        /*!< NULL or a pointer to a function to cleanup on destroy */
} I8080MemCallbacks;

/**
 * An empty callbacks structure
 */
extern const I8080MemCallbacks I8080MemEmptyCallbacks;

/**
 * Initialize an empty set of callbacks
 */
#define I8080MemCallbacksStructInit(R, W)    { .read = (R), .write = (W) }

/**
 * Create a new system memory object
 * The new memory space is created without any pages populated.  If
 * \p callbacks is not NULL, its callback functions and context pointers
 * are copied to the object for the life of the object.
 * @param callbacks     NULL of a pointer to the callback functions to
 *                      associate with the new object
 * @return              NULL on error or a newly-initialized system
 *                      memory
 */
I8080MemRef I8080MemCreate(const I8080MemCallbacks *callbacks, const void *context);

/**
 * Destroy a system memory object
 * Deallocates all memory pages and the \p mem object itself.
 * @param mem           the system memory object to destroy
 */
void I8080MemDestroy(I8080MemRef mem);

/**
 * Modify the system memory object's callbacks
 * Change the callbacks and context associated with \p mem.
 * @param mem           the system memory object to modify
 * @param callbacks     the new callbacks
 * @param context       the new context to go with the callbacks
 */
void I8080MemSetCallbacks(I8080MemRef mem, const I8080MemCallbacks *callbacks, const void *context);

/**
 * Reset a system memory object
 * Restores \p mem to its starting state.
 * @param mem           the system memory object to reset
 */
void I8080MemReset(I8080MemRef mem);

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

/**
 * Read a byte from memory
 * Reads a byte from the system memory \p mem.  For instances that have
 * a read arbiter callback present, the byte may actually be provided
 * by the arbiter and not actual present in memory per se.
 * @param mem           the system memory object
 * @param addr          the address to be read
 * @return              the byte read from \p addr
 */
uint8_t I8080MemRead(I8080MemRef mem, I8080Addr_t addr);

/**
 * Write a byte to memory
 * Writes a \p byte to the system memory \p mem.  For instances that have
 * a write arbiter callback present, the action may not actually transfer
 * the \p byte to system memory.
 * @param mem           the system memory object
 * @param addr          the address to be written
 * @return              the byte to write to \p addr
 */
void I8080MemWrite(I8080MemRef mem, I8080Addr_t addr, uint8_t byte);

/**
 * Write a byte to memory and read and return the result
 * Combines a \ref I8080MemWrite() followed by a \ref I8080MemRead()
 * of memory address \p addr.
 * @param mem           the system memory object
 * @param addr          the address to be written
 * @return              the byte read from \p addr after writing
 */
uint8_t I8080MemWriteThrough(I8080MemRef mem, I8080Addr_t addr, uint8_t byte);

/**
 * Context data structure for an unmapped memory segment
 * Used in conjunction with the \ref I8080MemUnmappedCallbacks
 * to force a range of addresses to be unwritable and return
 * a static byte value when read.
 *
 * The next_callbacks and next_context allow additional callbacks
 * to be chained with this one if it does NOT handle the
 * operation.
 */
typedef struct {
    I8080Addr_t         base_address;           /*!< address of the first unmapped byte in the segment */
    I8080Addr_t         unmapped_size;          /*!< the number of bytes in the segment */
    uint8_t             unmapped_byte;          /*!< the byte value to be returned when the segment is read */
    //
    const I8080MemCallbacks *next_callbacks;
    const void              *next_context;
} I8080MemUnmappedContext_t;

/**
 * Unmapped segment callbacks
 * Address of the callbacks that implement the \ref I8080MemUnmappedContext_t
 * operations.
 */
extern const I8080MemCallbacks *I8080MemUnmappedCallbacks;

/**
 * Optional behaviors for \p I8080MemROMContext_t instances
 * Do not set any bits other than those described here!
 */
typedef enum {
    kI8080MemROMOptsDeallocateAtCleanup = 0b1   /*!< the caller can clear this bit to prevent the ROM from being
                                                     deallocated when the \p I8080MemROMContext_t is destroyed */
} I8080MemROMOpts_t;

/**
 * Context data structure for a read-only memory segment
 * Used in conjunction with the \ref I8080MemROMCallbacks
 * to associate a static, immutable sequence of bytes with
 * a range of addresses in the system memory space.
 *
 * Instances of this context MUST be dynamically allocated;
 * a set of functions exist in the API to assist in this
 * task.  If you absolutely MUST have a static instance,
 * just be sure to clear the \ref kI8080MemROMOptsDeallocateAtCleanup
 * bit in the \p options bitvector!
 *
 * The next_callbacks and next_context allow additional callbacks
 * to be chained with this one if it does NOT handle the
 * operation.
 */
typedef struct {
    I8080MemROMOpts_t           options;    /*!< options */
    I8080Addr_t                 rom_addr;   /*!< address at which the ROM should be mapped */
    I8080Addr_t                 rom_size;   /*!< the number of bytes in the ROM image; zero implies the full 64 KiB */
    uint8_t                     *rom_image; /*!< pointer to the array of bytes that comprise the ROM image */
    //
    const I8080MemCallbacks *next_callbacks;
    const void              *next_context;
} I8080MemROMContext_t;

/**
 * Type of a pointer to a ROM image
 */
typedef I8080MemROMContext_t * I8080MemROMContextPtr;

/**
 * Allocate a ROM image with bytes in memory
 * If \p should_copy is false, then the \p bytes_len bytes at \p bytes is simply
 * wrapped by a \ref I8080MemROMContext_t; if \p should_copy is true, then the
 * \ref I8080MemROMContext_t is allocated such that \p bytes is copied into it.
 *
 * If \p bytes is NULL then a buffer of \p bytes_len is allocated and the \p rom_image
 * field in the returned \p I8080MemROMContextPtr can be modified as desired.
 * @param rom_addr      address at which the ROM should be mapped
 * @param bytes         pointer to the bytes comprising the ROM; if NULL, a buffer
 *                      will be created and zeroed and the caller can modify it as
 *                      desired
 * @param bytes_len     number of bytes in the ROM image; zero (0) implies the full
 *                      64 KiB address space
 * @param should_copy   if false then the returned \ref I8080MemROMContextPtr merely wraps
 *                      the \p bytes pointer -- which means \p bytes cannot be free'd
 *                      by the caller while the \p I8080MemROMContextPtr is in-use!
 * @return              pointer to the allocated and initialized I8080MemROMContext_t,
 *                      NULL on error
 */
I8080MemROMContextPtr I8080MemROMWithBytes(I8080Addr_t rom_addr, const void *bytes, I8080Addr_t bytes_len, bool should_copy);

/**
 * Allocate a ROM image with bytes from an open file
 * Create a \ref I8080MemROMContext_t that wraps at most \p bytes_len bytes read from the
 * open FILE, \p fptr.  A memory buffer is created as part of the \ref I8080MemROMContext_t
 * to hold the bytes.
 * @param rom_addr      address at which the ROM should be mapped
 * @param fptr          the file from which the bytes will be read
 * @param bytes_len     number of bytes in the ROM image; zero (0) implies the full
 *                      64 KiB address space
 * @return              pointer to the allocated and initialized I8080MemROMContext_t, NULL
 *                      on error
 */
I8080MemROMContextPtr I8080MemROMWithFilePtr(I8080Addr_t rom_addr, FILE *fptr, I8080Addr_t bytes_len);

/**
 * Allocate a ROM image with the contents of a file
 * Create a \ref I8080MemROMContext_t that wraps at most \p 64 KiB read from the file
 * at \p filepath.  A memory buffer is created as part of the \ref I8080MemROMContext_t
 * to hold the bytes.
 * @param rom_addr      address at which the ROM should be mapped
 * @param filepath      the file from which the ROM bytes should be read
 * @return              pointer to the allocated and initialized I8080MemROMContext_t, NULL
 *                      on error
 */
I8080MemROMContextPtr I8080MemROMWithFile(I8080Addr_t rom_addr, const char *filepath);

/**
 * Allocate a ROM image with a range of bytes from a file
 * Similar to \ref I8080MemROMWithFile(), but rather than reading bytes from
 * the start of the file the read can be repositioned using the \p offset
 * argument.  At most 64 KiB will be read.
 * @param rom_addr      address at which the ROM should be mapped
 * @param filepath      the file from which the ROM bytes should be read
 * @param offset        offset at which the ROM should be read from the
 *                      file
 * @param length        number of bytes to read from the file; zero (0) implies the full
 *                      64 KiB address space
 * @return              pointer to the allocated and initialized I8080MemROMContext_t, NULL
 *                      on error
 */
I8080MemROMContextPtr I8080MemROMWithByteRangeInFile(I8080Addr_t rom_addr, const char *filepath, off_t offset, I8080Addr_t length);

#ifdef HAS_MMAP
/**
 * Allocate a ROM image by mapping a file into virtual memory
 * Bytes in the file associated with file descriptor \p fd are mapped into the
 * native system's virtual address space.  The byte range (up to 64 KiB) starting
 * at \p offset in the file is in-scope.
 * @param rom_addr      address at which the ROM should be mapped
 * @param filepath      the file from which the ROM bytes should be read
 * @param offset        offset at which the ROM should be read from the
 *                      file
 * @param length        number of bytes to read from the file; zero (0) implies the full
 *                      64 KiB address space
 * @return              pointer to the allocated and initialized I8080MemROMContext_t,
 *                      NULL on error
 */
I8080MemROMContextPtr I8080MemROMWithMappedFile(I8080Addr_t rom_addr, int fd, off_t offset, I8080Addr_t length);
#endif

/**
 * ROM segment callbacks
 * Address of the callbacks that implement the \ref I8080MemROMContext_t
 * operations.
 */
extern const I8080MemCallbacks *I8080MemROMCallbacks;

#endif /* __I8080MEMORY_H__ */
