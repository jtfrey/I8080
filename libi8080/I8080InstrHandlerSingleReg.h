/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 single-register instructions APIs.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080INSTRSINGLEREG_H__
#define __I8080INSTRSINGLEREG_H__

#include "I8080Instructions.h"

extern const I8080InstrHandler_t I8080InstrHandlerSingleRegIncDec;

extern const I8080InstrHandler_t I8080InstrHandlerSingleRegAccum;

#endif /* __I8080INSTRSINGLEREG_H__ */
