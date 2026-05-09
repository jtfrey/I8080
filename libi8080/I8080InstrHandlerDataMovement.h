/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 data-movement instructions APIs.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080INSTRDATAMOVEMENT_H__
#define __I8080INSTRDATAMOVEMENT_H__

#include "I8080Instructions.h"

extern const I8080InstrHandler_t I8080InstrHandlerDataMovementMov;

extern const I8080InstrHandler_t I8080InstrHandlerDataMovementAccum;

extern const I8080InstrHandler_t I8080InstrHandlerDataMovementMVI;

extern const I8080InstrHandler_t I8080InstrHandlerDataMovementLXI;

#endif /* __I8080INSTRDATAMOVEMENT_H__ */
