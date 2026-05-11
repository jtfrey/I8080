/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Devices backed by files.
 */

#include "I8e8eFileDeviceObj.h"
#include "I8080Logging.h"

I8e8eFileDeviceObj_t*
I8e8eFileDeviceObjParse(
    const char      *in_str
)
{
    I8e8eFileDeviceObj_t    *filedev_obj = NULL;
    const char              *in_str_orig = in_str, *filepath_end;
    I8080DeviceMode         dev_mode = 0;
    int                     dev_id = (int)kI8080DevBusRegisterIdNextAvail, filepath_len;
    char                    fopen_mode[8] = "a+";
    
    // Move ahead to the colon that delimits the path:
    while ( *in_str && (*in_str != ':') ) in_str++;
    if ( ! *in_str ) {
        ERROR("Incomplete file device specification: %s", in_str_orig);
        return NULL;
    }
    filepath_end = in_str++;
    filepath_len = filepath_end - in_str_orig;
    
    // Set the selected i/o modes:
    do {
        // Next character must be an i or o:
        switch ( *in_str ) {
            case 'i':
            case 'I':
                dev_mode |= kI8080DeviceModeInput;
                in_str++;
                break;
            case 'o':
            case 'O':
                dev_mode |= kI8080DeviceModeOutput;
                in_str++;
                break;
        }
    } while ( *in_str && strchr("iIoO", *in_str) );
    
    // Get the optional pieces:
    while ( *in_str && strchr("#@", *in_str) ) {
        switch ( *in_str++ ) {
            case '#': {
                char        *endptr;
                long        v = strtol(in_str, &endptr, 0);
                
                if ( endptr == in_str ) {
                    ERROR("Invalid device id in file device specification: %s", in_str_orig);
                    return NULL;
                }
                if ( v > 0xFF ) {
                    ERROR("Device id 0x%X exceeds maximum of 0xFF: %s", in_str_orig);
                    return NULL;
                }
                if ( v == 0 ) {
                    ERROR("Device id 0 is used by the TTY: %s", in_str_orig);
                    return NULL;
                }
                dev_id = (v < 0) ? (int)kI8080DevBusRegisterIdNextAvail : v;
                in_str = endptr;
                break;
            }
            case '@': {
                int     i = 0;
                
                while ( i < 7 ) {
                    if ( ! *in_str || *in_str == '#' ) break;
                    fopen_mode[i++] = *in_str++;
                }
                if ( *in_str && *in_str != '#' ) {
                    ERROR("Invalid fopen mode in file device specification: %s", in_str_orig);
                    return NULL;
                }
                fopen_mode[i] = '\0';
                break;
            }
        }
    }
    if ( *in_str ) {
        ERROR("Invalid file device specification: %s", in_str_orig);
        ERROR("                                   %*s^", (in_str - in_str_orig), "");
        return NULL;
    }
    
    filedev_obj = (I8e8eFileDeviceObj_t*)calloc(1, sizeof(I8e8eFileDeviceObj_t) + filepath_len + 1);
    if ( ! filedev_obj ) {
        ERROR("Unable to allocate file device struct (errno=%d)", errno);
        return NULL;
    }
    memcpy(filedev_obj->filepath, in_str_orig, filepath_len);
    filedev_obj->filepath[filepath_len] = '\0';
    filedev_obj->mode = dev_mode;
    filedev_obj->dev_id = dev_id;
    memcpy(filedev_obj->fopen_mode, fopen_mode, sizeof(fopen_mode));
    return filedev_obj;
}

