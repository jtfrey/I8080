/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 direct-addressing instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080INSTRDIRECTADDR_H__
#define __I8080INSTRDIRECTADDR_H__

#include "I8080Instructions.h"

extern const I8080InstrHandler_t I8080InstrHandlerDirectAddr;

#endif /* __I8080INSTRDIRECTADDR_H__ */
