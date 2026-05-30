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

#ifndef __I8E8EBSMOBJ_H__
#define __I8E8EBSMOBJ_H__

#include "I8080BSM.h"
#include "I8e8eMemObj.h"

typedef struct I8e8eBSMObj {
    struct I8e8eBSMObj      *link;
    I8080Addr_t             base_page;
    I8080Addr_t             n_pages;
    I8080DevBusRegisterId   dev_id;
    I8080BSMContextPtr      bsm_context;
    I8080MemMapperRef_t     mapper_ref;
    I8080Device_t           device_callbacks;
} I8e8eBSMObj_t;

I8e8eBSMObj_t* I8e8eBSMObjParse(I8e8eROMMappingMode_t mapping_mode, const char *in_str);

bool I8e8eBSMObjRegister(I8e8eBSMObj_t *bsm_objs, I8080SystemPtr sys8080);
void I8e8eBSMObjDestroy(I8e8eBSMObj_t *bsm_objs);

#endif /* __I8E8EBSMOBJ_H__ */
