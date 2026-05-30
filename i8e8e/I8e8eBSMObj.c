/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * BSM object helpers.
 *
 */

#include "I8e8eBSMObj.h"
#include "I8080Logging.h"

I8e8eBSMObj_t*
I8e8eBSMObjParse(
    I8e8eROMMappingMode_t   mapping_mode,
    const char              *in_str
)
{
    I8e8eBSMObj_t       *out_bsm;
    const char          *in_str_orig = in_str;
    const char          *p_start, *p_end;
    long                v;
    int                 base;
    char                *endptr;
    
    I8080Addr_t             base_page, n_pages = 1;
    I8080DevBusRegisterId   dev_id = kI8080DevBusRegisterIdNextAvail;
    int                     n_components = 0;
    
    // Scan-ahead to the first separator character we expect
    // to find:
    p_start = p_end = in_str;
    while ( *p_end && ! strchr("+#:", *p_end) ) p_end++;
    if ( p_end == p_start || ! *p_end ) {
        ERROR("Invalid BSM spec: %s", in_str_orig);
        return NULL;
    }
    // Use strtol base 0 (hex/oct/dec ok) or force 16 if a
    // "$" leads the string:
    base = 0;
    if ( *p_start == '$' ) p_start++, base=16;
    v = strtol(p_start, (char**)&endptr, base);
    if ( endptr != p_end ) {
        ERROR("Invalid start page in BSM spec: %s", in_str_orig);
        return NULL;
    }
    // We're looking for a page number, so keep the value 8-bit:
    if ( v & ~0xFFUL ) {
        ERROR("Start page $%X is out of bounds: %s", v, in_str_orig);
        return NULL;
    }
    base_page = v;
    // Continue on from the end of the parsed page number:
    p_start = p_end;
    
    //
    // Scan for the optional page count and device id fields:
    //
    while ( *p_start != ':' ) {
        switch ( *p_start ) {
            case '+': {
                //
                // Page count -- treated the same as the page number
                // above:
                //
                p_start++;
                base = 0;
                if ( *p_start == '$' ) p_start++, base=16;
                v = strtol(p_start, (char**)&endptr, base);
                if ( endptr == p_start ) {
                    ERROR("Invalid page count in BSM spec: %s", in_str_orig);
                    return NULL;
                }
                if ( v & ~0xFFUL ) {
                    ERROR("Page count $%X is out of bounds: %s", v, in_str_orig);
                    return NULL;
                }
                n_pages = v;
                p_start = endptr;
                break;
            }
            case '#': {
                //
                // Device id -- also handled the same as the page number
                // and count:
                //
                p_start++;
                base = 0;
                if ( *p_start == '$' ) p_start++, base=16;
                v = strtol(p_start, (char**)&endptr, base);
                if ( endptr == p_start ) {
                    ERROR("Invalid device id in BSM spec: %s", in_str_orig);
                    return NULL;
                }
                if ( v & ~0xFFUL ) {
                    ERROR("Device id $%X is out of bounds: %s", v, in_str_orig);
                    return NULL;
                }
                dev_id = v;
                p_start = endptr;
                break;
            }
            case '\0':
                ERROR("Invalid BSM spec, no filename(s) present: %s", in_str_orig);
                return NULL;
            default:
                ERROR("Invalid character in BSM spec: %s", in_str_orig);
                ERROR("                              %*s^", p_start - in_str_orig, "");
                return NULL;
        }
    }
    
    // We've got everything leading up to the component files; count how many
    // components:
    p_end = p_start++;
    n_components = 0;
    while ( (p_end = strchr(p_end, ':')) ) n_components++, p_end++;
    
    // Allocate the BSM info object:
    out_bsm = (I8e8eBSMObj_t*)calloc(1, sizeof(I8e8eBSMObj_t));
    if( ! out_bsm ) {
        ERROR("Failed allocating BSM info object (errno=%d)", errno);
        return NULL;
    } else {
        I8080BSMBankOpts_t      bank_opts[n_components];
        I8080HostMemoryRef      bank_memories[n_components];
        
        out_bsm->base_page = base_page;
        out_bsm->n_pages = n_pages;
        out_bsm->dev_id = dev_id;
    
        // Go back and fill-in the memory objects; p_start still points to
        // the first filename:
        base = 0;
        while ( base < n_components ) {
            const char      *filename;
            
            bank_opts[base] = 0;
            p_end = p_start;
            while ( *p_end && ! strchr("+:", *p_end) ) p_end++;
            if ( p_end == p_start ) {
                ERROR("Zero-length filename in BSM spec: %s", in_str_orig);
                ERROR("                                 %*s^", p_end + 1 - in_str_orig, "");
                while ( base-- ) {
                    I8080HostMemoryRelease(bank_memories[base]);
                }
                free((void*)out_bsm);
                return NULL;
            }
            if ( *p_end == '+' ) {
                // Handle the component options:
                switch ( *(p_end + 1) ) {
                    case 'r':
                    case 'R':
                        break;
                    case 'w':
                    case 'W':
                        bank_opts[base] |= kI8080BSMBankOptsIsWritable;
                        break;
                    default:
                        ERROR("Invalid R/W character in BSM spec: %s", in_str_orig);
                        ERROR("                                  %*s^", p_end + 1 - in_str_orig, "");
                        while ( base-- ) {
                            I8080HostMemoryRelease(bank_memories[base]);
                        }
                        free((void*)out_bsm);
                        return NULL;
                }
            }
            
            // Copy the filename substring if it doesn't appear at the end of the string:
            filename = ( *p_end == ':' ) ? strndup(p_start, p_end - p_start) : p_start;
            
            // Create the memory object:
            bank_memories[base] = I8080HostMemoryCreateWithContentsOfNamedFile(filename, (mapping_mode == kI8e8eROMMappingModeMMap));
            if ( ! bank_memories[base] ) {
                if ( filename != p_start ) free((void*)filename);
                while ( base-- ) {
                    I8080HostMemoryRelease(bank_memories[base]);
                }
                free((void*)out_bsm);
                return NULL;
            }
            if ( filename != p_start ) free((void*)filename);
            p_start = p_end;
            if ( *p_start == ':' ) p_start++;
            base++;
        }
        //
        // Create the BSM object from the memory objects and
        // options we've accumulated:
        //
        INFO("Loaded %d memory image(s) for BSM context", n_components);
        out_bsm->bsm_context = I8080BSMContextCreateWithBanks(n_pages, n_components, bank_memories, bank_opts);
        if ( ! out_bsm->bsm_context ) {
            while ( base-- ) {
                I8080HostMemoryRelease(bank_memories[base]);
            }
            free((void*)out_bsm);
            ERROR("Failure while creating BSM object from component banks");
            return NULL;
        }
        //
        // I8080BSMContextCreateWithBanks() retains each memory
        // object, so we need to relese our reference:
        //
        while ( base-- ) I8080HostMemoryRelease(bank_memories[base]);
        
        //
        // Fill-in the rest of the BSM fields:
        //
        out_bsm->mapper_ref.addr_range = I8080AddrRangeCreate(base_page << 8, n_pages << 8);
        out_bsm->mapper_ref.callbacks = *I8080MemBSMCallbacks;
        out_bsm->mapper_ref.context = out_bsm->bsm_context;
        out_bsm->device_callbacks = *I8080DeviceBSMInOut;
    }    
    return out_bsm;
}

//

bool
I8e8eBSMObjRegister(
    I8e8eBSMObj_t   *bsm_objs,
    I8080SystemPtr  sys8080
)
{
    while ( bsm_objs ) {
        if ( ! I8080MemRegisterMapper(sys8080->sysmem, &bsm_objs->mapper_ref) ) {
            ERROR("Failed to register BSM mapper %p with system memory %p", bsm_objs, sys8080->sysmem);
            return NULL;
        }
        if ( ! I8080DevBusRegisterDevice(sys8080->devbus, bsm_objs->dev_id, &bsm_objs->device_callbacks, bsm_objs->bsm_context) ) {
            ERROR("Failed to register BSM i/o device %p with device bus %p", bsm_objs, sys8080->devbus);
            return NULL;
        }
        bsm_objs = bsm_objs->link;
    }
    return true;
}

//

void
I8e8eBSMObjDestroy(
    I8e8eBSMObj_t   *bsm_objs
)
{
    while ( bsm_objs ) {
        I8e8eBSMObj_t   *next = bsm_objs->link;
        free((void*)bsm_objs);
        bsm_objs = next;
    }
}
