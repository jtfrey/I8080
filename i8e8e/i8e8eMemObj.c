/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Memory object helpers.
 *
 */

#include "I8e8eMemObj.h"
#include "I8080Logging.h"

I8e8eMemObj_t*
I8e8eMemObjParse(
    I8e8eMemObjType_t   type,
    const char          *in_str
)
{
    I8e8eMemObj_t  *out_bytes_in;
    const char      *in_str_orig = in_str;
    bool            is_addr_set = false;
    
    if ( type == kI8e8eMemObjTypeData || type == kI8e8eMemObjTypeROM ) {
        int         file_path_len;
        
        while ( *in_str && ! strchr(":#@", *in_str) ) in_str++;
        // No NUL character yet, and there must be at least one
        // character other than the field separator:
        if ( ! *in_str || (in_str == in_str_orig) ) {
            ERROR("Invalid byte source: %s", in_str_orig);
            return NULL;
        }
        // Add the length of the <filepath> field plus NUL terminator:
        file_path_len = in_str - in_str_orig;
        // Allocate the struct:
        out_bytes_in = (I8e8eMemObj_t*)calloc(1, sizeof(I8e8eMemObj_t) + file_path_len + 1);
        if ( ! out_bytes_in ) {
            ERROR("Unable to allocate byte source struct (errno=%d)", errno);
            return NULL;
        }
        
        // Fill-in the type:
        out_bytes_in->obj_type = type;
        
        // Fill-in the filepath:
        memcpy((void*)&out_bytes_in->file_path, (const void*)in_str_orig, file_path_len);
        out_bytes_in->file_path[file_path_len] = '\0';
        
        while ( *in_str ) {
            const char      *endptr;
            
            switch ( *in_str++ ) {
                case ':': {
                    long        v;
                    int         base = 0;
                    
                    if ( *in_str == '$' ) in_str++, base=16;
                    v = strtol(in_str, (char**)&endptr, base);
                    if ( endptr == in_str ) {
                        ERROR("Invalid <offset> value in byte source: %s", in_str_orig);
                        free((void*)out_bytes_in);
                        return NULL;
                    }
                    out_bytes_in->file_offset = v;
                    break;
                }
                case '#': {
                    long        v;
                    int         base = 0;
                    
                    if ( *in_str == '$' ) in_str++, base=16;
                    v = strtol(in_str, (char**)&endptr, base);
                    if ( endptr == in_str ) {
                        ERROR("Invalid <len> value in byte source: %s", in_str_orig);
                        free((void*)out_bytes_in);
                        return NULL;
                    }
                    out_bytes_in->file_length = v;
                    break;
                }
                case '@': {
                    I8080Addr_t     lo = 0x0000, hi = 0x0000;
                    
                    if ( ! I8080AddrParse(in_str, &lo, &endptr) ) {
                        ERROR("Invalid <addr> value in byte source: %s", in_str_orig);
                        free((void*)out_bytes_in);
                        return NULL;
                    }
                    if ( *endptr ) {
                        if ( *endptr != '-' ) {
                            ERROR("Invalid <addr> range in byte source: %s", in_str_orig);
                            free((void*)out_bytes_in);
                            return NULL;
                        }
                        if ( ! I8080AddrParse(++endptr, &hi, &endptr) ) {
                            ERROR("Invalid <addr2> value in byte source: %s", in_str_orig);
                            free((void*)out_bytes_in);
                            return NULL;
                        }
                    }
                    if ( *endptr ) {
                        ERROR("Invalid byte source, extra characters: %s", in_str_orig);
                        ERROR("                                      %*s^", endptr - in_str_orig, "");
                        free((void*)out_bytes_in);
                        return NULL;
                    }
                    if ( hi == 0x0000 ) {
                        out_bytes_in->mapper_data.addr_range = I8080AddrRangeCreate(lo, hi);
                    } else {
                        out_bytes_in->mapper_data.addr_range = I8080AddrRangeCreateWithBounds(lo, hi);
                    }
                    is_addr_set = true;
                    break;
                }
                default:
                    ERROR("Invalid byte source: %s", in_str_orig);
                    ERROR("                    %*s^", (in_str - in_str_orig), "");
                    free((void*)out_bytes_in);
                    return NULL;
            }
            in_str = endptr;
        }
        if ( ! is_addr_set ) {
            ERROR("No <addr> present in byte source: %s", in_str_orig);
            free((void*)out_bytes_in);
            return NULL;
        }
    } else if ( type == kI8e8eMemObjTypeUnmapped ) {
        I8080AddrRange_t    addr_range = I8080AddrRangeCreate(0, 0);
        uint8_t             byte;
        long                v;
        int                 base = 0;
        const char          *endptr;
        
        if ( *in_str != '-' ) {
            // Low address:
            if ( ! I8080AddrParse(in_str, &addr_range.base, &endptr) ) {
                ERROR("Invalid low address in unmapped specification: %s", in_str_orig);
                return NULL;
            }
            in_str = endptr;
        }
        if ( *in_str++ != '-' ) {
            ERROR("Invalid unmapped specification: %s", in_str_orig);
            return NULL;
        }
        if ( *in_str && *in_str != ':' ) {
            // High address:
            I8080Addr_t     hi;
            
            if ( ! I8080AddrParse(in_str, &hi, &endptr) ) {
                ERROR("Invalid high address in unmapped specification: %s", in_str_orig);
                return NULL;
            }
            addr_range = I8080AddrRangeCreateWithBounds(addr_range.base, hi);
            in_str = endptr;
        }
        if ( ! *in_str || *in_str++ != ':' ) {
            ERROR("Invalid unmapped specification (missing static byte): %s", in_str_orig);
            return NULL;
        }
        
        if ( *in_str == '$' ) in_str++, base = 16;
        v = strtol(in_str, (char**)&endptr, base);
        if ( endptr == in_str ) {
            ERROR("Invalid unmapped specification: %s", in_str_orig);
            return NULL;
        }
        if ( v < 0 || v > 255 ) {
            byte = (v < 0) ? 0x00 : 0xFF;
            WARNING("Static byte value for unmapped segment was %ld, clamped to 0x%02hhX", v, byte);
        }
        
        // Allocate the struct:
        out_bytes_in = (I8e8eMemObj_t*)calloc(1, sizeof(I8e8eMemObj_t));
        if ( ! out_bytes_in ) {
            ERROR("Unable to allocate unmapped segment struct (errno=%d)", errno);
            return NULL;
        }
        out_bytes_in->obj_type = kI8e8eMemObjTypeUnmapped;
        out_bytes_in->mapper_data.addr_range = addr_range;
        out_bytes_in->unmapped_byte = byte;
    } else {
        ERROR("Invalid memory object type: %d", type);
    }
    return out_bytes_in;
}

//

bool
I8e8eROMMappingModeParse(
    const char              *in_str,
    I8e8eROMMappingMode_t   *out_mode
)
{
    if ( strcasecmp(in_str, "alloc") == 0 ) {
        *out_mode = kI8e8eROMMappingModeAlloc;
        return true;
    }
    if ( strcasecmp(in_str, "mmap") == 0 ) {
        *out_mode = kI8e8eROMMappingModeMMap;
        return true;
    }
    return false;
}

//

bool
I8e8eMemObjPreBootRegister(
    I8e8eMemObj_t           *mem_objs,
    I8e8eROMMappingMode_t   rom_mapmode,
    I8080SystemPtr          sys8080
)
{
    while ( mem_objs ) {
        I8080AddrRange_t        paged_addr = I8080AddrRangeToPages(mem_objs->mapper_data.addr_range);
        
        switch ( mem_objs->obj_type ) {
            case kI8e8eMemObjTypeData:
                /* Data needs to be loaded AFTER the system memory resets */
                break;
                
            case kI8e8eMemObjTypeROM: {
                mem_objs->rom_context.rom_image = I8080HostMemoryCreateWithRangeOfBytesInNamedFile(
                                mem_objs->file_path, mem_objs->file_offset, mem_objs->file_length,
                                (rom_mapmode == kI8e8eROMMappingModeMMap));
                if ( ! mem_objs->rom_context.rom_image ) {
                    ERROR("Unable to allocate ROM context (errno=%d)", errno);
                    return false;
                }
                mem_objs->rom_context.rom_name = mem_objs->file_path;
                mem_objs->rom_context.page_count = paged_addr.length;
                mem_objs->mapper_data.callbacks = *I8080MemROMCallbacks;
                mem_objs->mapper_data.context = &mem_objs->rom_context;
                if ( ! I8080MemRegisterMapper(sys8080->sysmem, &mem_objs->mapper_data) ) {
                    return false;
                }
                INFO("Created new %s ROM image @ $%04hX - $%04hX", 
                    (rom_mapmode == kI8e8eROMMappingModeAlloc) ? "allocated buffer" : "memory-mapped",
                    mem_objs->mapper_data.addr_range.base, I8080AddrRangeEndAddr(mem_objs->mapper_data.addr_range));
                break;
            }
            case kI8e8eMemObjTypeUnmapped: {
                mem_objs->mapper_data.callbacks = *I8080MemUnmappedSegmentCallbacks;
                mem_objs->mapper_data.context = (const void*)(uintptr_t)mem_objs->unmapped_byte;
                if ( ! I8080MemRegisterMapper(sys8080->sysmem, &mem_objs->mapper_data) ) {
                    return false;
                }
                INFO("Created new unmapped segment @ $%04hX - $%04hX (byte = 0x%02hhX)", 
                        mem_objs->mapper_data.addr_range.base, I8080AddrRangeEndAddr(mem_objs->mapper_data.addr_range), mem_objs->unmapped_byte);
                break;
            }
        }
        mem_objs = mem_objs->link;
    }
    return true;
}

//

bool
I8e8eMemObjPostBootRegister(
    I8e8eMemObj_t           *mem_objs,
    I8e8eROMMappingMode_t   rom_mapmode,
    I8080SystemPtr          sys8080
)
{
    while ( mem_objs ) {
        switch ( mem_objs->obj_type ) {
            case kI8e8eMemObjTypeData: {
                FILE        *fptr = fopen(mem_objs->file_path, "rb");
                bool        full_length = (mem_objs->file_length == 0);
                I8080Addr_t addr = mem_objs->mapper_data.addr_range.base;
                size_t      n_bytes = 0;
                
                if ( mem_objs->mapper_data.addr_range.length > 0 ) {
                    // An address range was provided, does it limit the read?
                    if ( full_length || (mem_objs->mapper_data.addr_range.length < mem_objs->file_length) ) {
                        mem_objs->file_length = mem_objs->mapper_data.addr_range.length;
                        full_length = false;
                    } 
                }
                
                if ( ! fptr ) {
                    ERROR("Unable to open file `%s` to load data to system memory (errno=%d)",
                            mem_objs->file_path, errno);
                    return false;
                }
                if ( mem_objs->file_offset ) {
                    if ( fseeko(fptr, mem_objs->file_offset, SEEK_SET) ) {
                        ERROR("Unable to move to offset %lld in `%s` to load data to system memory (errno=%d)",
                            mem_objs->file_offset, mem_objs->file_path, errno);                        
                        fclose(fptr);
                        return false;
                    }
                }
                while ( full_length || (mem_objs->file_length > 0) ) {
                    uint8_t     byte;
                    
                    if ( fread(&byte, 1, 1, fptr) != 1 ) break;
                    I8080MemWrite(sys8080->sysmem, addr++, byte);
                    mem_objs->file_length--, n_bytes++;
                }
                if ( ! full_length && mem_objs->file_length > 0 ) {
                    ERROR("Unable to read all bytes from `%s` (%lld unread)", mem_objs->file_path, mem_objs->file_length);
                    fclose(fptr);
                    return false;
                }
                fclose(fptr);
                INFO("Added %lld bytes to system memory starting at $%04hX\n", n_bytes, mem_objs->mapper_data.addr_range.base);
                break;
            }
            case kI8e8eMemObjTypeROM:
            case kI8e8eMemObjTypeUnmapped: 
                break;
        }
        mem_objs = mem_objs->link;
    }
    return true;
}

//

void
I8e8eMemObjDestroy(
    I8e8eMemObj_t   *mem_objs
)
{
    while ( mem_objs ) {
        I8e8eMemObj_t     *next_obj = mem_objs->link;
        
        free((void*)mem_objs);
        mem_objs = next_obj;
    }
}
