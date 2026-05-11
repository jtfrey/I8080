/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Devices backed by files.
 */

#ifndef __I8E8EFILEDEVICEOBJ_H__
#define __I8E8EFILEDEVICEOBJ_H__

#include "I8080DeviceIO.h"

typedef struct I8e8eFileDeviceObj {
    struct I8e8eFileDeviceObj   *link;
    I8080DeviceMode             mode;
    int                         dev_id;
    char                        fopen_mode[8];
    I8080Device_t               device;
    I8080DeviceFileContext_t    device_context;
    char                        filepath[];
} I8e8eFileDeviceObj_t;

I8e8eFileDeviceObj_t* I8e8eFileDeviceObjParse(const char *in_str);

#endif /* __I8E8EFILEDEVICEOBJ_H__ */
