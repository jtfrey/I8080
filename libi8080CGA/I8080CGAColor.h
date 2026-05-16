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
 *
 * The Apple II palettes come from info found via Google.
 */
typedef enum {
    kI8080CGAPaletteIdDefault           = 0,
    kI8080CGAPaletteIdRealCGA           = kI8080CGAPaletteIdDefault,
    kI8080CGAPaletteIdAppleIILoRes      = 1,
    kI8080CGAPaletteIdAppleIIHiRes      = 2,
    kI8080CGAPaletteId4BitRGB           = 3,
    kI8080CGAPaletteIdRustGold          = 4,
    kI8080CGAPaletteIdNESClassic        = 5,
    kI8080CGAPaletteIdCC29              = 6,
    kI8080CGAPaletteIdBlkEx96           = 7,
    kI8080CGAPaletteIdStrangeFantasy    = 8,
    kI8080CGAPaletteIdSunfall12         = 9,
    kI8080CGAPaletteIdSpookyFruit       = 10,
    kI8080CGAPaletteIdLoSpec500         = 11,
    kI8080CGAPaletteIdThirtyOne         = 12,   /* Actually "31" on LoSpec */
    kI8080CGAPaletteIdResurrect64       = 13,
    kI8080CGAPaletteIdAAPSplendor       = 14,
    kI8080CGAPaletteId1BitMonitorGlow   = 15,
    kI8080CGAPaletteIdMax
} I8080CGAPaletteId_t;

/**
 * Textual names for the palette ids
 * The strings are indexed matching the ids in the \ref I8080CGAPaletteId_t
 * enumeration.
 */
extern const char* I8080CGAPaletteIdStrs[];

/**
 * Parse a palette id from a string
 */
bool I8080CGAPaletteIdParse(const char *in_str, I8080CGAPaletteId_t *palette_id, const char **end_str);

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
