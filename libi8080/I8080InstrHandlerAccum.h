/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 accumulator instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080INSTRACCUM_H__
#define __I8080INSTRACCUM_H__

#include "I8080Instructions.h"

extern const I8080InstrHandler_t I8080InstrHandlerAccum;

extern const I8080InstrHandler_t I8080InstrHandlerAccumRotate;

extern const I8080InstrHandler_t I8080InstrHandlerAccumImmed;

#endif /* __I8080INSTRACCUM_H__ */
