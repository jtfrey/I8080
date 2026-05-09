/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 register pairs instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080INSTRREGPAIRS_H__
#define __I8080INSTRREGPAIRS_H__

#include "I8080Instructions.h"

extern const I8080InstrHandler_t I8080InstrHandlerRegPairsStack;

extern const I8080InstrHandler_t I8080InstrHandlerRegPairsArith;

extern const I8080InstrHandler_t I8080InstrHandlerRegPairsExchange;

#endif /* __I8080INSTRREGPAIRS_H__ */
