/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 native host memory API.
 */
 
#ifndef __I8080HOSTMEMORY_H__
#define __I8080HOSTMEMORY_H__

#include "I8080Config.h"

/**
 * Kind of memory region associated with an I8080HostMemory object
 */
typedef enum {
    kI8080HostMemoryKindWrappedPointer, /*!< the object holds an externally-allocated pointer and size; when the object
                                             is released, the pointer is NOT deallocated */
    kI8080HostMemoryKindOwnedPointer,   /*!< the object holds an externally-allocated pointer and size; when the object
                                             is released, the pointer is deallocated using free() */
    kI8080HostMemoryKindAllocation,     /*!< the object holds an internally-allocated pointer and size; when the object
                                             is released, the pointer is deallocated */
    kI8080HostMemoryKindMMap,           /*!< the object holds a pointer to a memory-mapped file containing the given
                                             number of bytes; when the object is released, the pointer is unmapped */
    kI8080HostMemoryKindMax
} I8080HostMemoryKind_t;

/**
 * An object representing a region of host memory
 * For a \ref kI8080HostMemoryKindAllocation object, the \p mem_bytes pointer refers
 * to a chunk of memory allocated as part of the \ref I8080HostMemory_t object itself,
 * whereas a \ref kI8080HostMemoryKindMMap object is a mmap()'ed pointer to an OS-managed
 * chunk of memory.  The remaining kinds both refer to an externally-allocated and
 * -managed memory region, with the only difference being whether or not the
 * \ref I8080HostMemory_t deallocates that region itself or abdicates that responsibility
 * to...something else.
 */
typedef struct {
    unsigned int            ref_count;      /*!< the number of references held to this memory object */
    I8080HostMemoryKind_t   mem_kind;       /*!< the kind of memory object represented by this object */
    size_t                  mem_size;       /*!< the number of bytes in the region of memory represeted by this object */
    void*                   mem_bytes;      /*!< pointer to the region of bytes represented by this object */
} I8080HostMemory_t;

/**
 * Type of a reference to an I8080HostMemory object
 */
typedef I8080HostMemory_t * I8080HostMemoryRef;

/**
 * Create an I8080HostMemory object that wraps a size and pointer
 * Given a pointer to some bytes external to this API, creates and returns
 * an instance that wraps that external memory buffer.  When the object
 * is eventually deallocated, the pointer is NOT deallocated.
 * @param mem_size      number of bytes in the memory region at \p mem_ptr
 * @param mem_ptr       the pointer to wrap
 * @return              an I8080HostMemory object that wraps \p mem_ptr
 */
I8080HostMemoryRef I8080HostMemoryCreateWithWrappedPointer(size_t mem_size, void *mem_ptr);

/**
 * Create an I8080HostMemory object that wraps a size and pointer
 * The only difference between this function and \ref I8080HostMemoryCreateWithWrappedPointer()
 * is that in this function, the returned object takes ownership of \p mem_ptr and will
 * deallocate it when the object is deallocated.
 * @param mem_size      number of bytes in the memory region at \p mem_ptr
 * @param mem_ptr       the pointer that will now be owned by the \ref I8080HostMemory
 *                      object returned
 * @return              an I8080HostMemory object that wraps (and owns) \p mem_ptr
 */
I8080HostMemoryRef I8080HostMemoryCreateWithOwnedPointer(size_t mem_size, void *mem_ptr);

/**
 * Create an I8080HostMemory object that allocates a memory region
 * A chunk of \p mem_size bytes is allocated and wrapped by the returned
 * I8080HostMemory object.
 * @param mem_size      the size of the allocated memory region
 * @return              an I8080HostMemory object that wraps an allocated
 *                      chunk of memory
 */
I8080HostMemoryRef I8080HostMemoryCreateWithSize(size_t mem_size);

/**
 * Create an I8080HostMemory object that contains bytes from a file
 * If \p use_mmap is false, then an I8080HostMemory object of size \p mem_size is
 * created and filled with up to \p mem_size bytes read from \p fd.  If there are
 * not enough bytes in the file to fill the object, the remainder will contain zeroes.
 *
 * If \p use_mmap is true — and the library was compiled with mmap() support — the 
 * region \p mem_size bytes long at the current read position in \p fd is mapped into
 * the virtual memory address space.  The OS will handle moving pages between memory
 * and the file as necessary.
 * @param fd            file descriptor holding desired bytes
 * @param mem_size      the number of bytes to read into memory from \p fd
 * @param use_mmap      if true, have the OS map the file region into virtual memory
 *                      and automatically swap pages back and forth
 * @return              an I8080HostMemory object that wraps the bytes from the file
 */
I8080HostMemoryRef I8080HostMemoryCreateWithContentsOfFileDescriptor(int fd, size_t mem_size, bool use_mmap);

/**
 * Create an I8080HostMemory object that contains bytes from a file
 * Convenience function that calls \ref I8080HostMemoryCreateWithContentsOfFileDescriptor()
 * with the file descriptor associated with \p fptr.
 * @param fptr          stdio file holding desired bytes
 * @param mem_size      the number of bytes to read into memory from \p fptr
 * @param use_mmap      if true, have the OS map the file region into virtual memory
 *                      and automatically swap pages back and forth
 * @return              an I8080HostMemory object that wraps the bytes from the file
 */
I8080HostMemoryRef I8080HostMemoryCreateWithContentsOfFilePtr(FILE *fptr, size_t mem_size, bool use_mmap);

/**
 * Create an I8080HostMemory object that contains the contents of a named file
 * Attempts to open the file at \p filename and read (or map) it's entire contents
 * into memory.
 * @param filename      path to the file
 * @param use_mmap      if true, have the OS map the file contents into virtual memory
 *                      and automatically swap pages back and forth
 * @return              an I8080HostMemory object that wraps the file content
 */
I8080HostMemoryRef I8080HostMemoryCreateWithContentsOfNamedFile(const char *filename, bool use_mmap);

/**
 * Create an I8080HostMemory object that contains a portion of a named file
 * Similar to \ref I8080HostMemoryCreateWithContentsOfNamedFile() but only the range of
 * \p length bytes at \p offset are used.
 * @param filename      path to the file
 * @param offset        offset to the first byte in the range
 * @param length        number of bytes in the range
 * @param use_mmap      if true, have the OS map the range of bytes into virtual memory
 *                      and automatically swap pages back and forth
 * @return              an I8080HostMemory object that wraps the file content
 */
I8080HostMemoryRef I8080HostMemoryCreateWithRangeOfBytesInNamedFile(const char *filename, off_t offset, size_t length, bool use_mmap);

/**
 * Increase the reference count of an I8080HostMemory object
 * Increase the number of references to \p host_mem and return a pointer to the
 * object.
 * @param host_mem      the I8080HostMemory object
 * @return              an additional reference to \p host_mem
 */
I8080HostMemoryRef I8080HostMemoryRetain(I8080HostMemoryRef host_mem);

/**
 * Decrease the reference count of an I8080HostMemory object
 * Decrease the number of references to \p host_mem.  When zero references
 * remain, the object is destroyed.
 * @param host_mem      the I8080HostMemory object
 */
void I8080HostMemoryRelease(I8080HostMemoryRef host_mem);

#endif /* __I8080HOSTMEMORY_H__ */
