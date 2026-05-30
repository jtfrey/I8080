/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 Game pad.
 */
 
#ifndef __I8080DPAD_H__
#define __I8080DPAD_H__

#include "I8080DeviceIO.h"

typedef enum __attribute__((packed)) {
    kI8080DPadBitsRight     = 0b1,
    kI8080DPadBitsLeft      = 0b10,
    kI8080DPadBitsDown      = 0b100,
    kI8080DPadBitsUp        = 0b1000,
    kI8080DPadBitsStart     = 0b10000,
    kI8080DPadBitsSelect    = 0b100000,
    kI8080DPadBitsB         = 0b1000000,
    kI8080DPadBitsA         = 0b10000000,
} I8080DPadBits_t;

/** 
 * I/O input device definition for a simple d-pad controller
 * featuring four directional buttons, two action buttons,
 * and a start and select button.
 *
 * No context data necessary, just register these
 * callbacks on an input channel.
 */
extern const I8080DevicePtr I8080DeviceDPadIn;

#endif /* __I8080DPAD_H__ */
