/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines color as used with the I8080 Curses Graphics Adapter.
 */
 
#ifndef __I8080CGACOLOR_H__
#define __I8080CGACOLOR_H__

#include "I8080Config.h"

/**
 * Pre-built color palettes
 * A collection of 24bpp (8bpc) color palettes downloaded from
 * https://lospec.com/.
 */
typedef enum {
    kI8080CGAPaletteIdDefault = 0,
    kI8080CGAPaletteId4BitRGB = kI8080CGAPaletteIdDefault,
    kI8080CGAPaletteIdRustGold,
    kI8080CGAPaletteIdNESClassic,
    kI8080CGAPaletteIdCC29,
    kI8080CGAPaletteIdBlkEx96,
    kI8080CGAPaletteIdStrangeFantasy,
    kI8080CGAPaletteIdSunfall12,
    kI8080CGAPaletteIdSpookyFruit,
    kI8080CGAPaletteIdLoSpec500,
    kI8080CGAPaletteId31,
    kI8080CGAPaletteResurrect64,
    kI8080CGAPaletteId1BitMonitorGlow,
    kI8080CGAPaletteAAPSplendor,
    kI8080CGAPaletteIdMax
} I8080CGAPaletteId_t;

/**
 * A 24bpp (8bpc) color
 * Three 8-bit channels, R, G, B.
 */
typedef struct {
    uint8_t     r, g, b;
} I8080CGAColor_t;

/**
 * A color palette
 * A palette is an array of \ref I8080CGAColor_t records.  It is meant
 * to be loaded into the curses environment and referenced by index.
 * The color at index 0 is actually color 1 in the system -- color 0
 * is reserved.  So a palette can have at most 254 color definitions.
 */
typedef struct {
    uint8_t             n_colors;   /*!< the number of colors in the palette */
    I8080CGAColor_t     colors[];   /*!< the array of color definitions */
} I8080CGAPalette_t;

/**
 * Type of a pointer to a pre-defined palette
 */
typedef const I8080CGAPalette_t * I8080CGAPalettePtr;

/**
 * Pre-defined palettes
 * Array of pointers to the pre-defined color palettes.  Individual
 * palettes can be referenced by their index in \ref I8080CGAPaletteId_t.
 */
extern const I8080CGAPalettePtr I8080CGAPalettes[kI8080CGAPaletteIdMax];

#endif /* __I8080CGACOLOR_H__ */
