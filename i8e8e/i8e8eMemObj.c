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
        int         filepath_len;
        
        while ( *in_str && ! strchr(":#@", *in_str) ) in_str++;
        // No NUL character yet, and there must be at least one
        // character other than the field separator:
        if ( ! *in_str || (in_str == in_str_orig) ) {
            ERROR("Invalid byte source: %s", in_str_orig);
            return NULL;
        }
        // Add the length of the <filepath> field plus NUL terminator:
        filepath_len = in_str - in_str_orig;
        // Allocate the struct:
        out_bytes_in = (I8e8eMemObj_t*)calloc(1, sizeof(I8e8eMemObj_t) + filepath_len + 1);
        if ( ! out_bytes_in ) {
            ERROR("Unable to allocate byte source struct (errno=%d)", errno);
            return NULL;
        }
        
        // Fill-in the type:
        out_bytes_in->obj_type = type;
        
        // Fill-in the filepath:
        memcpy((void*)&out_bytes_in->filepath, (const void*)in_str_orig, filepath_len);
        out_bytes_in->filepath[filepath_len] = '\0';
        
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
                    out_bytes_in->offset = v;
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
                    out_bytes_in->len = v;
                    break;
                }
                case '@':
                    if ( ! I8080AddrParse(in_str, &out_bytes_in->addr, &endptr) ) {
                        ERROR("Invalid <addr> value in byte source: %s", in_str_orig);
                        free((void*)out_bytes_in);
                        return NULL;
                    }
                    is_addr_set = true;
                    break;
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
        I8080Addr_t     lo = 0x0000, hi = 0xFFFF;
        uint8_t         byte;
        long            v;
        int             base = 0;
        const char      *endptr;
        
        if ( *in_str != '-' ) {
            // Low address:
            if ( ! I8080AddrParse(in_str, &lo, &endptr) ) {
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
            if ( ! I8080AddrParse(in_str, &hi, &endptr) ) {
                ERROR("Invalid high address in unmapped specification: %s", in_str_orig);
                return NULL;
            }
            in_str = endptr;
        }
        if ( ! *in_str || *in_str++ != ':' ) {
            ERROR("Invalid unmapped specification (missing static byte): %s", in_str_orig);
            return NULL;
        }
        
        if ( lo >= hi ) {
            ERROR("Invalid memory range in unmapped specification: $%04hX - $%04hX", lo, hi);
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
        out_bytes_in->len = (hi - lo) + 1;
        out_bytes_in->addr = lo;
        out_bytes_in->byte = byte;
    } else {
        ERROR("Invalid memory object type: %d", type);
    }
    return out_bytes_in;
}
