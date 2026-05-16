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
    I8080MemMapperRef_t             mapper_data;
    
    uint8_t                         unmapped_byte;
    
    off_t                           file_offset;
    off_t                           file_length;
    char                            file_path[];
} I8e8eMemObj_t;

I8e8eMemObj_t* I8e8eMemObjParse(I8e8eMemObjType_t type, const char *in_str);

#endif /* __I8E8EMEMOBJ_H__ */
