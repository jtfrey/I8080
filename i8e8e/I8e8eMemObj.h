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

#include "I8080System.h"
#include "I8080Memory.h"

typedef enum {
    kI8e8eMemObjTypeData,
    kI8e8eMemObjTypeROM,
    kI8e8eMemObjTypeUnmapped
} I8e8eMemObjType_t;

typedef enum {
    kI8e8eROMMappingModeAlloc,
    kI8e8eROMMappingModeMMap
} I8e8eROMMappingMode_t;

bool I8e8eROMMappingModeParse(const char *in_str, I8e8eROMMappingMode_t *out_mode);

typedef struct I8e8eMemObj {
    struct I8e8eMemObj              *link;
    I8e8eMemObjType_t               obj_type;
    I8080MemMapperRef_t             mapper_data;
    
    union {
        uint8_t                     unmapped_byte;
        I8080MemROMContext_t        rom_context;
    };
    
    off_t                           file_offset;
    off_t                           file_length;
    char                            file_path[];
} I8e8eMemObj_t;

I8e8eMemObj_t* I8e8eMemObjParse(I8e8eMemObjType_t type, const char *in_str);

bool I8e8eMemObjPreBootRegister(I8e8eMemObj_t *mem_objs, I8e8eROMMappingMode_t rom_mapmode, I8080SystemPtr sys8080);
bool I8e8eMemObjPostBootRegister(I8e8eMemObj_t *mem_objs, I8e8eROMMappingMode_t rom_mapmode, I8080SystemPtr sys8080);
void I8e8eMemObjDestroy(I8e8eMemObj_t *mem_objs);

#endif /* __I8E8EMEMOBJ_H__ */
