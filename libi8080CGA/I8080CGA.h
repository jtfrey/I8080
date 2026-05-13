/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 Curses Graphics Adapter.
 */
 
#ifndef __I8080CGA_H__
#define __I8080CGA_H__

#include "I8080Memory.h"
#include "I8080CGAColor.h"
#include <curses.h>

/**
 * Initialize curses
 * A basic curses library initialization routine.
 */
void I8080CGAInit(void);

/**
 * Shutdown curses
 * A basic curses library shutdown routine.
 */
void I8080CGAShutdown(void);


/**
 * CGA display mode
 * The display can operate in text mode (where the bytes in the
 * array are ASCII characters) or in graphics mode (where the
 * bytes in the array are color indices).
 *
 * Black-and-white graphics mode uses space characters in A_NORMAL
 * versus A_REVERSE mode.
 *
 * Color graphics mode uses space characters with a COLOR_PAIR
 * associated with it.
 */
typedef enum __attribute__((packed)) {
    kI8080CGAModeText           = 0x00,     /*!< text mode */
    kI8080CGAModeBWGraphics     = 0x01,     /*!< black-and-white graphics mode */
    kI8080CGAModeClrGraphics    = 0x02,     /*!< color graphics mode */
    
    kI8080CGAModeBasicModesMask = 0x03,     /*!< mask for no color */
    kI8080CGAModeAllModesMask   = 0x07      /*!< mask for all modes */
} I8080CGAMode_t;

/**
 * CGA operation trigger
 * The state of the display can be altered via byte-based operations
 * communicated through the adapter's "op" register.
 *
 * Some operations require data to be written to other adapter
 * registers prior to the operation's being triggered.  Others
 * will introduce data into those registers when triggered which
 * the user code can subsequently read.
 */
typedef enum __attribute__((packed)) {
    kI8080CGAOpNoOp         = 0x00, /*!< zero doesn't do anything... */
    kI8080CGAOpClear        = 0x01, /*!< clear the screen */
    kI8080CGAOpClearRow     = 0x02, /*!< clear the row in .y field */
    kI8080CGAOpClearCol     = 0x03, /*!< clear the row in .x field */
    
    kI8080CGAOpWriteNChars  = 0x10, /*!< the u16lo/u16hi registers should have the
                                         address of a string of characters; the .i
                                         register is the number of characters; and
                                         the .x, .y registers have a screen coordinate */
    kI8080CGAOpWriteCStr    = 0x11, /*!< the u16lo/u16hi registers should have the
                                         address of a string of characters; the .x, .y
                                         registers have a screen coordinate */
                                                 
    kI8080CGAOpGetColorRGB  = 0x20, /*!< fill RGB registers with color .i intensities */
    kI8080CGAOpSetColorRGB  = 0x21, /*!< set color .i with intensities in RGB registers */
    
    kI8080CGAOpFill         = 0x80, /*!< fill with the ASCII character in the lower
                                         7 bits of the operation */
} I8080CGAOp_t;

/**
 * CGA register index
 * The index of each of the CGA display registers.
 */
typedef enum {
    kI8080CGARegisterSupModes = 0,
    kI8080CGARegisterWidth,
    kI8080CGARegisterHeight,
    kI8080CGARegisterNColors,
    kI8080CGARegisterRedraw,
    kI8080CGARegisterX,
    kI8080CGARegisterY,
    kI8080CGARegisterI,
    kI8080CGARegisterR,
    kI8080CGARegisterG,
    kI8080CGARegisterB,
    kI8080CGARegisterU16Lo,
    kI8080CGARegisterU16Hi,
    kI8080CGARegisterMode,
    kI8080CGARegisterOp
} I8080CGARegister_t;

/**
 * CGA registers
 * The registers exist in the first 16 bytes of the memory page
 * where the adapter has been mapped in the emulator's memory space.
 * Each is an 8-bit value; some are read-only (r/o) and others
 * facilitate interchange with user code.
 *
 * The actual array of screen elements thus exists at an offset of 16
 * bytes from the mapping address:  e.g. if mapped at $2000, the array
 * is based at $2010.  Screen elements are ordered row-major:  at
 * $2010 is coordinate (0,0), $2011 is (1, 0), etc.  "But why is there
 * no array declared in the data structure?" you ask.  Curses itself
 * buffers the screen data so there's no need to replicate that here.
 * The array is virtual in the sense that user code read/write memory
 * operations are getch()/mvaddch() calls in the device implementation.
 */
typedef union {
    uint8_t                 R[16];          /*!< the register array */
    struct {
        uint8_t             supmodes;       /*!< (r/o) bit vector of supported modes */
        uint8_t             width;          /*!< (r/o) the width of the screen */
        uint8_t             height;         /*!< (r/o) the height of the screen */
        uint8_t             ncolors;        /*!< (r/o) the number of supported colors */
        uint8_t             redraw;         /*!< when set to zero, redraw is deferred until
                                                 this register is set to non-zero */
        uint8_t             x;              /*!< x-coordinate register */
        uint8_t             y;              /*!< y-coordinate register */
        uint8_t             i;              /*!< index register */
        uint8_t             r;              /*!< red-intensity register */
        uint8_t             g;              /*!< green-intensity register */
        uint8_t             b;              /*!< blue-intensity register */
        uint8_t             u16lo;          /*!< low-byte of a 16-bit value */
        uint8_t             u16hi;          /*!< high-byte of a 16-bit value */
        I8080CGAMode_t      mode;           /*!< the mode in which the display is operating */
        I8080CGAOp_t        op;             /*!< operation trigger */
    };
} I8080CGARegisters_t;

/**
 * CGA memory callback context
 * The context that must be associated with a \ref I8080CGACallbacks memory
 * callback.
 */
typedef struct {
    I8080Addr_t             base_addr;              /*!< where the CGA is mapped in memory */
    I8080CGARegisters_t     rgstrs;                 /*!< the CGA registers */
    WINDOW                  *wndw;                  /*!< the curses window for the display */
    const void              *rch;                   /*!< internal only -- this will be setup at
                                                         instance creation */
    const void              *wch;                   /*!< internal only -- this will be setup at
                                                         instance creation */
    short                   saved_colors[4*254];    /*!< internal only -- this will be setup at
                                                         instance creation; a 4 x n_color array of
                                                         (1/0, R, G, B) values when colors are set */
    //
    const I8080MemCallbacks *next_callbacks;
    const void              *next_context;
} I8080CGAContext_t;

/**
 * Type of a pointer to a CGA context
 */
typedef I8080CGAContext_t * I8080CGAContextPtr;

/**
 * Create a CGA callbacks context
 * Allocates and initializes a \ref I8080CGAContext_t to accompany
 * a I8080CGACallbacks memory overlay.  The context will be mapped
 * into the emulator's address space at \p base_addr and wrap the
 * curses window, \p wndw.  Read-only registers (like width, height)
 * will be determined and set at this time.  The \p wndw will be set
 * in the desired display \p mode.
 *
 * Initial loading of colors can be done using this API's
 * \ref I8080CGAContextSetColor() function; doing so will ensure
 * that when the context is cleaned-up original colors for the
 * user's terminal will be restored.
 * @param base_addr         memory address where the adapter will live
 * @param mode              initial display mode
 * @param wndw              the curses window in which to display
 * @return                  pointer to the newly-initialized
 *                          \ref I8080CGAContext_t or NULL on error
 */
I8080CGAContextPtr I8080CGAContextCreate(I8080Addr_t base_addr, I8080CGAMode_t mode, WINDOW *wndw);

/**
 * Retrieve a color's channel intensities
 * The red, green, and blue 8-bit intensities of the curses color at
 * index \p color is retrieved.
 * @param cga               the CGA context
 * @param color             the color index
 * @param r                 pointer to the byte that will accept the red
 *                          intensity
 * @param g                 pointer to the byte that will accept the green
 *                          intensity
 * @param b                 pointer to the byte that will accept the blue
 *                          intensity
 */
void I8080CGAContextGetColor(I8080CGAContextPtr cga, int color, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * Set a color's channel intensities
 * The red, green, and blue 8-bit intensities of the curses color at
 * index \p color is set.
 * @param cga               the CGA context
 * @param color             the color index
 * @param r                 the 8-bit red intensity
 * @param g                 the 8-bit green intensity
 * @param b                 the 8-bit blue intensity
 */
void I8080CGAContextSetColor(I8080CGAContextPtr cga, int color, uint8_t r, uint8_t g, uint8_t b);

/**
 * Restore curses original color palette
 * The saved original color intensities present in \p cga are restored.
 * @param cga               the CGA context
 */
void I8080CGAContextRestoreColors(I8080CGAContextPtr cga);

/**
 * Set curses colors and pairs from a palette
 * The color definitions present in \p palette are loaded into the curses
 * runtime environment.  The zeroeth (first) color in the palette is assigned
 * to curses color 1 and that color is assigned to pair 1.
 * @param cga               the CGA context
 * @param palette           pointer to the color palette
 */
void I8080CGAContextLoadColorPalette(I8080CGAContextPtr cga, I8080CGAPalettePtr palette);

/**
 * Read screen element at coordinate
 * The screen element -- ASCII character or color index -- at coordinate
 * (x, y) in the window associatd with \p cga is returned.
 * @param cga               the CGA context
 * @param x                 x-coordinate
 * @param y                 y-coordinate
 * @return                  the screen element
 */
uint8_t I8080CGAContextReadXY(I8080CGAContextPtr cga, int x, int y);

/**
 * Write screen element at coordinate
 * The screen element -- an ASCII character or color index -- at
 * coordinate (x, y) in the window associatd with \p cga is set to
 * \p byte.
 * @param cga               the CGA context
 * @param x                 x-coordinate
 * @param y                 y-coordinate
 * @param byte              the screen element to set
 */
void I8080CGAContextWriteXY(I8080CGAContextPtr cga, int x, int y, uint8_t byte);

/**
 * CGA callbacks
 * Address of the callbacks that implement the \ref I8080CGAContext_t
 * operations.
 */
extern const I8080MemCallbacks *I8080CGACallbacks;

#endif /* __I8080CGA_H__ */
