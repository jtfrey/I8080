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

#ifndef __I8E8EMEMOBJ_H__
#define __I8E8EMEMOBJ_H__

#include "I8080Memory.h"

typedef enum {
    kI8e8eMemObjTypeData,
    kI8e8eMemObjTypeROM,
    kI8e8eMemObjTypeUnmapped
} I8e8eMemObjType_t;

typedef struct I8e8eMemObj {
    struct I8e8eMemObj              *link;
    I8e8eMemObjType_t               obj_type;
    off_t                           offset;
    off_t                           len;
    I8080Addr_t                     addr;
    uint8_t                         byte;
    union {
        I8080MemROMContext_t        *rom;
        I8080MemUnmappedContext_t   *unmapped;
    } context;
    I8080MemCallbacks               callbacks;
    char                            filepath[];
} I8e8eMemObj_t;

I8e8eMemObj_t* I8e8eMemObjParse(I8e8eMemObjType_t type, const char *in_str);

#endif /* __I8E8EMEMOBJ_H__ */
