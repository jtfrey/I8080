/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 native host memory API.
 */

#include "I8080HostMemory.h"
#include "I8080Logging.h"
#ifdef HAS_MMAP
#   include <sys/mman.h>
#endif

static
I8080HostMemory_t*
I8080HostMemoryAlloc(
    size_t              attached_bytes
)
{
    I8080HostMemory_t   *new_host_mem;
    
    new_host_mem = (I8080HostMemory_t*)calloc(1, sizeof(I8080HostMemory_t) + attached_bytes);
    if ( new_host_mem ) {
        new_host_mem->ref_count = 1;
        if ( attached_bytes ) {
            new_host_mem->mem_size = attached_bytes;
            new_host_mem->mem_bytes = ((void*)new_host_mem) + sizeof(I8080HostMemory_t);
        } else {
            new_host_mem->mem_size = 0L;
            new_host_mem->mem_bytes = NULL;
        }
    }
    return new_host_mem;
}

//

I8080HostMemoryRef
I8080HostMemoryCreateWithWrappedPointer(
    size_t      mem_size,
    void        *mem_ptr
)
{
    I8080HostMemory_t   *new_host_mem = I8080HostMemoryAlloc(0L);
    
    if ( new_host_mem ) {
        new_host_mem->mem_kind = kI8080HostMemoryKindWrappedPointer;
        new_host_mem->mem_size = mem_size;
        new_host_mem->mem_bytes = mem_ptr;
    }
    return (I8080HostMemoryRef)new_host_mem;
}

//

I8080HostMemoryRef
I8080HostMemoryCreateWithOwnedPointer(
    size_t      mem_size,
    void        *mem_ptr
)
{
    I8080HostMemory_t   *new_host_mem = I8080HostMemoryAlloc(0L);
    
    if ( new_host_mem ) {
        new_host_mem->mem_kind = kI8080HostMemoryKindOwnedPointer;
        new_host_mem->mem_size = mem_size;
        new_host_mem->mem_bytes = mem_ptr;
    }
    return (I8080HostMemoryRef)new_host_mem;
}

//

I8080HostMemoryRef
I8080HostMemoryCreateWithSize(
    size_t      mem_size
)
{
    I8080HostMemory_t   *new_host_mem = I8080HostMemoryAlloc(mem_size);
    
    if ( new_host_mem ) {
        new_host_mem->mem_kind = kI8080HostMemoryKindAllocation;
    }
    return (I8080HostMemoryRef)new_host_mem;
}

//

I8080HostMemoryRef
I8080HostMemoryCreateWithContentsOfFileDescriptor(
    int             fd,
    size_t          mem_size,
    bool            use_mmap
)
{
    I8080HostMemory_t   *new_host_mem = NULL;
    off_t               src_offset = lseek(fd, 0, SEEK_CUR);
    
    if ( mem_size == 0 ) {
        struct stat finfo;
    
        if ( fstat(fd, &finfo ) ) {
            ERROR("Unable to get file metadata for descriptor %d (errno=%d)", fd, errno);
            return NULL;
        }
        mem_size = finfo.st_size - src_offset;
    }
    if ( use_mmap ) {
#ifndef HAS_MMAP
        ERROR("The I8080 library was not compiled with support for mmap().");   
#else
        off_t       src_offset = lseek(fd, 0, SEEK_CUR);
        void        *segment = mmap(NULL, mem_size, PROT_READ, MAP_PRIVATE | MAP_FILE, fd, src_offset);
            
        if ( segment != MAP_FAILED ) {
            new_host_mem = I8080HostMemoryAlloc(0L);
            if ( new_host_mem ) {
                new_host_mem->mem_kind = kI8080HostMemoryKindMMap;
                new_host_mem->mem_size = mem_size;
                new_host_mem->mem_bytes = segment;
            } else {
                ERROR("Failed to allocate I8080HostMemory object of size %ld for fd %d", mem_size, fd);
                munmap(segment, mem_size);
            }
        } else {
            ERROR("Failed to map %ld bytes from file descriptor %d (errno=%d)", mem_size, fd, errno);
        }
#endif
    } else {
        new_host_mem = I8080HostMemoryAlloc(mem_size);
        if ( new_host_mem ) {
            new_host_mem->mem_kind = kI8080HostMemoryKindAllocation;
            read(fd, new_host_mem->mem_bytes, mem_size);
        }
    }
    return (I8080HostMemoryRef)new_host_mem;
}

//

I8080HostMemoryRef
I8080HostMemoryCreateWithContentsOfFilePtr(
    FILE        *fptr,
    size_t      mem_size,
    bool        use_mmap
)
{
    return I8080HostMemoryCreateWithContentsOfFileDescriptor(fileno(fptr), mem_size, use_mmap);
}

//

I8080HostMemoryRef
I8080HostMemoryCreateWithContentsOfNamedFile(
    const char      *filename,
    bool            use_mmap
)
{
    I8080HostMemory_t   *new_host_mem = NULL;
    int                 fd = open(filename, O_RDONLY);
    
    if ( fd < 0 ) {
        ERROR("Failed opening named file '%s' for read (errno=%d)", filename, errno);
    } else {
        new_host_mem = I8080HostMemoryCreateWithContentsOfFileDescriptor(fd, 0L, use_mmap);
        close(fd);
    }
    return (I8080HostMemoryRef)new_host_mem;
}

//

I8080HostMemoryRef
I8080HostMemoryCreateWithRangeOfBytesInNamedFile(
    const char      *filename,
    off_t           offset,
    size_t          length,
    bool            use_mmap
)
{
    I8080HostMemory_t   *new_host_mem = NULL;
    int                 fd = open(filename, O_RDONLY);
    
    if ( fd < 0 ) {
        ERROR("Failed opening named file '%s' for read (errno=%d)", filename, errno);
    } else {
        if ( lseek(fd, offset, SEEK_SET) < 0 ) {
            ERROR("Failed to move to offset %ld in file '%s' for read (errno=%d)", offset, filename, errno);
        } else {
            new_host_mem = I8080HostMemoryCreateWithContentsOfFileDescriptor(fd, length, use_mmap);
            close(fd);
        }
    }
    return (I8080HostMemoryRef)new_host_mem;
}

//

I8080HostMemoryRef
I8080HostMemoryRetain(
    I8080HostMemoryRef  host_mem
)
{
    host_mem->ref_count++;
    return host_mem;
}

//

void
I8080HostMemoryRelease(
    I8080HostMemoryRef  host_mem
)
{
    if ( --host_mem->ref_count == 0 ) {
        switch ( host_mem->mem_kind ) {
            case kI8080HostMemoryKindWrappedPointer:
            case kI8080HostMemoryKindAllocation:
            case kI8080HostMemoryKindMax:
                break;
            case kI8080HostMemoryKindOwnedPointer:
                free((void*)host_mem->mem_bytes);
                break;
            case kI8080HostMemoryKindMMap:
#ifdef HAS_MMAP
                munmap((void*)host_mem->mem_bytes, host_mem->mem_size);
#endif
                break;
        }
        free((void*)host_mem);
    }
}
