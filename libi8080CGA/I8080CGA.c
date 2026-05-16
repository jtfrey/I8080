/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 Curses Graphics Adapter.
 */

#include "I8080CGA.h"
#include "I8080Logging.h"
#include <sys/ioctl.h>

//

typedef uint8_t (*I8080CGAReadChCallback)(chtype ch);

static uint8_t I8080CGAReadChText(chtype ch) { return (ch & A_CHARTEXT) & 0x7f; }
static uint8_t I8080CGAReadChBWGraphics(chtype ch) { return (ch & A_REVERSE) ? 1 : 0; }
static uint8_t I8080CGAReadChClrGraphics(chtype ch) { return PAIR_NUMBER(ch - 1); }

//

typedef chtype (*I8080CGAWriteChCallback)(uint8_t);

static chtype I8080CGAWriteChText(uint8_t byte) { return (chtype)(byte & 0x7f); }
static chtype I8080CGAWriteChBWGraphics(uint8_t byte) { return ((chtype)' ' | (byte ? A_REVERSE : 0)); }
static chtype I8080CGAWriteChClrGraphics(uint8_t byte) { return ((chtype)' ' | COLOR_PAIR(byte + 1)); }

//

typedef enum {
    kI8080CGAMapperStatusProvidedWindow     = 0b1,      /*!< the caller provided a curses WINDOW, not
                                                             coordinates for a WINDOW this context should
                                                             create */
    kI8080CGAMapperStatusCursesWasActive    = 0b10,     /*!< when the context was created curses was
                                                            already initialized and active */
    kI8080CGAMapperStatusCursesInited       = 0b100,    /*!< if the curses was not active but the context
                                                             has called initscr() et al. */
    kI8080CGAMapperStatusIsEnabled          = 0b1000,   /*!< set when the context has made the CGA display
                                                             enabled */
    kI8080CGAMapperStatusIsRedrawDeferred   = 0b10000,  /*!< set when redrawing has been deferred */
} I8080CGAMapperStatus_t;

//

#define I8080CGAMapperDrawNow(S)    (((S) & kI8080CGAMapperStatusIsRedrawDeferred) == 0)

//

typedef struct {
    I8080CGAMapperContext_t     public;                 /*!< the public portion of the context */
    I8080CGARegisters_t         rgstrs;                 /*!< the CGA registers */
    I8080CGAMapperStatus_t      status;                 /*!< the status bitmask for the context */
    I8080CGAReadChCallback      rch;                    /*!< the character read function associated with
                                                             the selected mode */
    I8080CGAWriteChCallback     wch;                    /*!< the character write function associated with
                                                             the selected mode */
    short                       saved_colors[4*254];    /*!< a 4 x n_color array of (1/0, R, G, B) tuples
                                                             indicating whether or not the color has been
                                                             modified in curses, and if so what the saved
                                                             original color was */
} I8080CGAMapperPrivateContext_t;

//

static
void
I8080CGAContextHandleModeChange(
    I8080CGAMapperPrivateContext_t  *cga,
    uint8_t                         new_mode
)
{
    bool            is_enabled = (cga->status & kI8080CGAMapperStatusIsEnabled) != 0;
    
    if ( is_enabled ) {
        uint8_t     mode_bit = 1 << new_mode;
        int         color_idx;
        
        // Make sure we're out of deferred update mode:
        cga->status &= ~kI8080CGAMapperStatusIsRedrawDeferred;
        
        // Restore any saved colors:
        I8080CGAMapperContextRestoreColors(&cga->public);
        
        if ( cga->rgstrs.supmodes & mode_bit ) {
            switch ( new_mode ) {
                case kI8080CGAModeText:
                    wbkgd(cga->public.wndw, 0);
                    cga->rch = I8080CGAReadChText;
                    cga->wch = I8080CGAWriteChText;
                    break;
                case kI8080CGAModeBWGraphics:
                    wbkgd(cga->public.wndw, (chtype)' ');
                    cga->rch = I8080CGAReadChBWGraphics;
                    cga->wch = I8080CGAWriteChBWGraphics;
                    break;
                case kI8080CGAModeClrGraphics:
                    wbkgd(cga->public.wndw, (chtype)' ' | COLOR_PAIR(0));
                    cga->rch = I8080CGAReadChClrGraphics;
                    cga->wch = I8080CGAWriteChClrGraphics;
                    if ( cga->public.color_palette ) {
                        I8080CGAMapperContextLoadColorPalette(&cga->public, cga->public.color_palette);
                        cga->rgstrs.ncolors = cga->public.color_palette->n_colors;
                    } else {
                        cga->rgstrs.ncolors = 16;
                    }
                    break;
            }
            wclear(cga->public.wndw);
            clear();
            wrefresh(cga->public.wndw);
            refresh();
            cga->rgstrs.mode = new_mode;
        }
    }
}

//

static
bool
I8080CGAContextHandleEnable(
    I8080CGAMapperPrivateContext_t  *cga
)
{
    bool            cga_enable = (cga->rgstrs.enable != 0),
                    was_enabled = (cga->status & kI8080CGAMapperStatusIsEnabled) == kI8080CGAMapperStatusIsEnabled;
                    
    if ( cga_enable != was_enabled ) {
        if ( cga_enable ) {
            bool    does_support_color;
            uint8_t mode;
            
            // Turn the display on:
            if ( ! (cga->status & kI8080CGAMapperStatusCursesWasActive) ) {
                if ( ! (cga->status & kI8080CGAMapperStatusCursesInited) ) {
                    initscr();
                    start_color();
                    noecho();
                    cbreak();
                    curs_set(0);
                    cga->status |= kI8080CGAMapperStatusCursesInited;
                } else {
                    // Reenter curses mode:
                    wrefresh(stdscr);
                }
            }
            
            does_support_color = has_colors();
            
            // Supported modes:
            cga->rgstrs.supmodes = does_support_color ? kI8080CGAModeAllModesMask : kI8080CGAModeBasicModesMask;
            
            // Is the selected mode valid?
            mode = (((1 << cga->public.initial_mode) & cga->rgstrs.supmodes) == 0) ? kI8080CGAModeText : cga->public.initial_mode;
            
            // Number of colors:
            cga->rgstrs.ncolors = does_support_color ? ((COLORS < 254) ? COLORS : 254) : 0;
            
            // Setup the window:
            if ( cga->status & kI8080CGAMapperStatusProvidedWindow ) {
                if ( cga->public.wndw == NULL ) cga->public.wndw = stdscr;
            } else {
                cga->public.wndw = newwin(cga->public.h, cga->public.w, cga->public.y, cga->public.x);
            }
            keypad(cga->public.wndw, TRUE);
            nodelay(cga->public.wndw, FALSE);
            clearok(cga->public.wndw, FALSE);
            leaveok(cga->public.wndw, TRUE);
            scrollok(cga->public.wndw, FALSE);
            immedok(cga->public.wndw, FALSE);
            curs_set(0);
            
            // Fill-in dimensions:
            getmaxyx(cga->public.wndw, cga->rgstrs.height, cga->rgstrs.width);
            
            // Update the status:
            cga->status = (cga->status & ~kI8080CGAMapperStatusIsRedrawDeferred) | kI8080CGAMapperStatusIsEnabled;
            
            // Force a mode change:
            I8080CGAContextHandleModeChange(cga, mode);
        } else {
            // Turn the display off:
            wclear(cga->public.wndw);
            if ( cga->public.wndw != stdscr ) {
                delwin(cga->public.wndw);
                cga->public.wndw = NULL;
            }
            refresh();
                
            // Restore any saved colors:
            I8080CGAMapperContextRestoreColors(&cga->public);
            
            if ( ! (cga->status & kI8080CGAMapperStatusCursesWasActive) ) {
                endwin();
            }
            cga->status &= ~(kI8080CGAMapperStatusIsRedrawDeferred | kI8080CGAMapperStatusIsEnabled);
        }
    }
    if ( cga_enable ) {
        bool        cga_defer = (cga->rgstrs.enable & 0b10000000) != 0,
                    was_deferred = (cga->status & kI8080CGAMapperStatusIsRedrawDeferred) == kI8080CGAMapperStatusIsRedrawDeferred;
        
        
        if ( cga_defer != was_deferred ) {
            if ( cga_defer ) {
                // Begin deferred updates:
                redrawwin(cga->public.wndw);
                cga->status |= kI8080CGAMapperStatusIsRedrawDeferred;
            } else {
                // End deferred updates:
                wnoutrefresh(cga->public.wndw);
                doupdate();
                cga->status &= ~kI8080CGAMapperStatusIsRedrawDeferred;
            }
        }
    } else {
        // Make sure the defer bit is NOT set in the register:
        cga->rgstrs.enable &= ~kI8080CGAMapperStatusIsRedrawDeferred;
    }
    return true;
}

//

static
bool
I8080CGAContextRead(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             *byte,
    const void          *context
)
{
    I8080CGAMapperPrivateContext_t *cga = (I8080CGAMapperPrivateContext_t*)context;
    I8080Addr_t             cga_addr = addr - range.base;
    
    /* Register? */
    if ( cga_addr < sizeof(I8080CGARegisters_t) ) {
        /* This is straightforward enough:  all registers are 8-bit
         * so we can just treat the address offset as an index into
         * the registers as an array of bytes.  Not quite so simple
         * for write!
         */
        *byte = cga->rgstrs.R[cga_addr];
        return true;
    } else {
        /* Map the address offset to the screen element array: */
        cga_addr -= sizeof(I8080CGARegisters_t);
        if ( cga_addr < (cga->rgstrs.width * cga->rgstrs.height) ) {
            /* Turn the address into the (x, y): */
            int         y = cga_addr / cga->rgstrs.width,
                        x = cga_addr % cga->rgstrs.width;
            *byte = ((I8080CGAReadChCallback)cga->rch)(mvwinch(cga->public.wndw, y, x));
            return true;
        }
    }
    return false;
}

//

static
bool
I8080CGAContextWrite(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             byte,
    const void          *context
)
{
    I8080CGAMapperPrivateContext_t *cga = (I8080CGAMapperPrivateContext_t*)context;
    I8080Addr_t             cga_addr = addr - range.base;
    
    /* Register? */
    if ( cga_addr < sizeof(I8080CGARegisters_t) ) {
        /* Handle each register: */
        switch ( cga_addr ) {
            case kI8080CGARegisterSupModes:
            case kI8080CGARegisterWidth:
            case kI8080CGARegisterHeight:
            case kI8080CGARegisterNColors:
                break;
                
            case kI8080CGARegisterX:
            case kI8080CGARegisterY:
            case kI8080CGARegisterI:
            case kI8080CGARegisterR:
            case kI8080CGARegisterG:
            case kI8080CGARegisterB:
            case kI8080CGARegisterU16Lo:
            case kI8080CGARegisterU16Hi:
                cga->rgstrs.R[cga_addr] = byte;
                break;
            
            case kI8080CGARegisterEnable:
                cga->rgstrs.enable = byte;
                I8080CGAContextHandleEnable(cga);
                break;
            
            case kI8080CGARegisterMode:
                if ( byte != cga->rgstrs.mode ) I8080CGAContextHandleModeChange(cga, byte);
                break;
                
            case kI8080CGARegisterOp:
                cga->rgstrs.op = byte;
                if ( (cga->rgstrs.op & kI8080CGAOpFill) == kI8080CGAOpFill ) {
                    chtype          row[cga->rgstrs.width], ch = ((I8080CGAReadChCallback)cga->wch)(cga->rgstrs.op & 0x7f);
                    int             y;
                    
                    /* Fill the row: */
                    for ( y = 0; y < cga->rgstrs.width; y++ ) row[y] = ch;
                    
                    
                    /* Fill the screen: */
                    if ( I8080CGAMapperDrawNow(cga->status) ) {
                        redrawwin(cga->public.wndw);
                    }
                    for ( y = 0; y < cga->rgstrs.height; y++ ) mvwaddchnstr(cga->public.wndw, y, 0, row, cga->rgstrs.width);
                    if ( I8080CGAMapperDrawNow(cga->status) ) {
                        wnoutrefresh(cga->public.wndw);
                        doupdate();
                    }
                } else {
                    switch ( cga->rgstrs.op ) {
                        case kI8080CGAOpClear:
                            wclear(cga->public.wndw);
                            if ( I8080CGAMapperDrawNow(cga->status) ) wrefresh(cga->public.wndw);
                            break;
                        case kI8080CGAOpClearRow: {
                            int         x;
                            
                            if ( I8080CGAMapperDrawNow(cga->status) ) {
                                redrawwin(cga->public.wndw);
                            }
                            mvwaddch(cga->public.wndw, cga->rgstrs.y, 0, 0);
                            for ( x = 1; x < cga->rgstrs.width; x++ ) waddch(cga->public.wndw, 0);
                            if ( I8080CGAMapperDrawNow(cga->status) ) {
                                wnoutrefresh(cga->public.wndw);
                                doupdate();
                            }
                            break;
                        }
                        case kI8080CGAOpClearCol: {
                            int         y;
                            
                            if ( I8080CGAMapperDrawNow(cga->status) ) {
                                redrawwin(cga->public.wndw);
                            }
                            for ( y = 0; y < cga->rgstrs.height; y++ )
                                mvwaddch(cga->public.wndw, y, cga->rgstrs.x, 0);
                            if ( I8080CGAMapperDrawNow(cga->status) ) {
                                wnoutrefresh(cga->public.wndw);
                                doupdate();
                            }
                            break;
                        }
                        case kI8080CGAOpWriteNChars: {
                            if ( cga->rgstrs.i > 0 ) {
                                I8080Addr_t addr = ((I8080Addr_t)cga->rgstrs.u16hi << 8) | cga->rgstrs.u16lo;
                                uint8_t     b = I8080MemRead(mem, addr++);
                                
                                if ( I8080CGAMapperDrawNow(cga->status) ) {
                                    redrawwin(cga->public.wndw);
                                }
                                mvwaddch(cga->public.wndw, cga->rgstrs.y, cga->rgstrs.x, ((I8080CGAReadChCallback)cga->wch)(b));
                                while ( --cga->rgstrs.i ) {
                                    b = I8080MemRead(mem, addr++);
                                    waddch(cga->public.wndw, ((I8080CGAReadChCallback)cga->wch)(b));
                                }
                                if ( I8080CGAMapperDrawNow(cga->status) ) {
                                    wnoutrefresh(cga->public.wndw);
                                    doupdate();
                                }
                                cga->rgstrs.u16hi = (addr & 0xFF00) >> 8,
                                cga->rgstrs.u16lo = (addr & 0xFF);
                            }
                            break;
                        }
                        case kI8080CGAOpWriteCStr: {
                            I8080Addr_t addr = ((I8080Addr_t)cga->rgstrs.u16hi << 8) | cga->rgstrs.u16lo;
                            uint8_t     b = I8080MemRead(mem, addr++);
                            
                            if ( b ) {
                                if ( I8080CGAMapperDrawNow(cga->status) ) {
                                    redrawwin(cga->public.wndw);
                                }
                                mvwaddch(cga->public.wndw, cga->rgstrs.y, cga->rgstrs.x, ((I8080CGAReadChCallback)cga->wch)(b));
                                while ( (b = I8080MemRead(mem, addr++)) ) {
                                    waddch(cga->public.wndw, ((I8080CGAReadChCallback)cga->wch)(b));
                                }
                                if ( I8080CGAMapperDrawNow(cga->status) ) {
                                    wnoutrefresh(cga->public.wndw);
                                    doupdate();
                                }
                            }
                            cga->rgstrs.u16hi = (addr & 0xFF00) >> 8,
                            cga->rgstrs.u16lo = (addr & 0xFF);
                            break;
                        }
                        case kI8080CGAOpGetColorRGB: {
                            I8080CGAMapperContextGetColor(&cga->public, cga->rgstrs.i, &cga->rgstrs.r, &cga->rgstrs.g, &cga->rgstrs.b);
                            break;
                        }
                        case kI8080CGAOpSetColorRGB: {
                            I8080CGAMapperContextSetColor(&cga->public, cga->rgstrs.i, cga->rgstrs.r, cga->rgstrs.g, cga->rgstrs.b);
                            break;
                        }
                        default:
                            break;
                    }
                }
                break;
        }
        return true;
    } else {
        /* Map the address offset to the screen element array: */
        cga_addr -= sizeof(I8080CGARegisters_t);
        if ( cga_addr < (cga->rgstrs.width * cga->rgstrs.height) ) {
            /* Turn the address into the (x, y): */
            int         y = cga_addr / cga->rgstrs.width,
                        x = cga_addr % cga->rgstrs.width;
            mvwaddch(cga->public.wndw, y, x, ((I8080CGAReadChCallback)cga->wch)(byte));
            if ( I8080CGAMapperDrawNow(cga->status) ) {
                wrefresh(cga->public.wndw);
            }
            return true;
        }
    }
    return false;
}

//

static
void
I8080CGAContextShutdown(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    const void          *context
)
{
    I8080CGAMapperPrivateContext_t *cga = (I8080CGAMapperPrivateContext_t*)context;
    
    cga->rgstrs.enable = 0;
    I8080CGAMapperContextRestoreColors(&cga->public);
    I8080CGAContextHandleEnable(cga);
}

//

const I8080MemMapperCallbacks __I8080CGAMapperCallbacks = {
            .mapper_name = "curses-graphics-adapter",
            .rewrite_addr = NULL,
            .read = I8080CGAContextRead,
            .write = I8080CGAContextWrite,
            .reset = NULL,
            .shutdown = I8080CGAContextShutdown
        };
const I8080MemMapperCallbacks *I8080CGAMapperCallbacks = &__I8080CGAMapperCallbacks;

//

void
I8080CGAInit(void)
{
    initscr();
    start_color();
	noecho();
	cbreak();
    clear();
}

//

void
I8080CGAShutdown(void)
{
    endwin();
}

//

I8080CGAMapperContextPtr
I8080CGAMapperContextCreateWithOriginAndSize(
    I8080CGAMode_t      mode,
    unsigned int        x,
    unsigned int        y,
    unsigned int        w,
    unsigned int        h
)
{
    I8080CGAMapperPrivateContext_t  *context = NULL;
    
    context = (I8080CGAMapperPrivateContext_t*)calloc(1, sizeof(I8080CGAMapperPrivateContext_t));
    if ( context ) {
        // Fill-in the window dimensions:
        context->public.x = x, context->public.y = y, context->public.w = w, context->public.h = h;
        
        // Fill-in the number of pages needed:
        context->public.n_pages_required = (w * h + 255) / 256;
        
        // Set the initial mode:
        context->public.initial_mode = mode & kI8080CGAModeAllModesMask;
        
        // Status word:
        context->status = 0;
        
        // Detect whether or not curses is active:
        if ( stdscr && ! isendwin() ) context->status |= kI8080CGAMapperStatusCursesWasActive;
    }
    return (I8080CGAMapperContextPtr)context;
}

//

I8080CGAMapperContextPtr
I8080CGAMapperContextCreateWithWindow(
    I8080CGAMode_t      mode,
    WINDOW              *wndw
)
{
    I8080CGAMapperPrivateContext_t  *context = NULL;
    
    context = (I8080CGAMapperPrivateContext_t*)calloc(1, sizeof(I8080CGAMapperPrivateContext_t));
    if ( context ) {
        // Fill-in the window:
        context->public.wndw = wndw;
        
        // Non-NULL window, we can set the page requirements:
        if ( wndw ) {
            int     h, w;
            getmaxyx(wndw, h, w);
            context->public.n_pages_required = (w * h + 255) / 256;
        } else {
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
            context->public.n_pages_required = (w.ws_row * w.ws_col + 255) / 256;
        }
        
        // Set the initial mode:
        context->public.initial_mode = mode & kI8080CGAModeAllModesMask;
        
        // Status word:
        context->status = kI8080CGAMapperStatusProvidedWindow;
        
        // Detect whether or not curses is active:
        if ( stdscr && ! isendwin() ) context->status |= kI8080CGAMapperStatusCursesWasActive;
    }
    return (I8080CGAMapperContextPtr)context;
}

//

void
I8080CGAMapperContextGetColor(
    I8080CGAMapperContextPtr  cga,
    int                 color,
    uint8_t             *r,
    uint8_t             *g,
    uint8_t             *b
)
{
    short               sr, sg, sb;
    color_content(color, &sr, &sg, &sb);
    *r = 255 * ((double)sr / 1000.0);
    *g = 255 * ((double)sg / 1000.0);
    *b = 255 * ((double)sb / 1000.0);
}

//

void
I8080CGAMapperContextSetColor(
    I8080CGAMapperContextPtr  cga,
    int                 color,
    uint8_t             r,
    uint8_t             g,
    uint8_t             b
)
{
    I8080CGAMapperPrivateContext_t *CGA = (I8080CGAMapperPrivateContext_t*)cga;
    short               sr = 1000 * ((double)r / 255.0),
                        sg = 1000 * ((double)g / 255.0),
                        sb = 1000 * ((double)b / 255.0);
    short               *s = &CGA->saved_colors[4 * (color - 1)];
    
    if ( ! *s ) {
        *s = 1;
        color_content(color, s + 1, s + 2, s + 3);
    }
    init_color(color, sr, sg, sb);
    init_pair(color, color, color);
}

//

void
I8080CGAMapperContextRestoreColors(
    I8080CGAMapperContextPtr  cga
)
{
    I8080CGAMapperPrivateContext_t *CGA = (I8080CGAMapperPrivateContext_t*)cga;
    int                 c = 1;
    short               *s = &CGA->saved_colors[0],
                        *s_end = s + sizeof(CGA->saved_colors);
                        
    while ( s < s_end ) {
        if ( *s ) init_color(c, *(s + 1), *(s + 2), *(s + 3));
        s += 4, c++;
    }
    memset(CGA->saved_colors, 0, sizeof(CGA->saved_colors));
}

//

void
I8080CGAMapperContextLoadColorPalette(
    I8080CGAMapperContextPtr  cga,
    I8080CGAPalettePtr  palette
)
{
    const I8080CGAColor_t   *colors = palette->colors;
    int                     color;
    
    I8080CGAMapperContextRestoreColors(cga);
    for ( color = 0; color < palette->n_colors; colors++ )
        I8080CGAMapperContextSetColor(cga, ++color, colors->r, colors->g, colors->b);
}

//

uint8_t
I8080CGAMapperContextGetRegister(
    I8080CGAMapperContextPtr    cga,
    I8080CGARegister_t          ridx
)
{
    uint8_t         byte;
    
    I8080CGAContextRead(NULL, I8080AddrRangeCreate(0x0000, 0xFFFF), (I8080Addr_t)ridx, &byte, cga);
    return byte;
}

//

bool
I8080CGAMapperContextSetRegister(
    I8080CGAMapperContextPtr    cga,
    I8080CGARegister_t          ridx,
    uint8_t                     byte
)
{
    return I8080CGAContextWrite(NULL, I8080AddrRangeCreate(0x0000, 0xFFFF), (I8080Addr_t)ridx, byte, cga);
}

//

void
I8080CGAMapperContextSetEnable(
    I8080CGAMapperContextPtr    cga,
    uint8_t                     enable
)
{
    I8080CGAMapperPrivateContext_t *CGA = (I8080CGAMapperPrivateContext_t*)cga;
    
    CGA->rgstrs.enable = enable;
    I8080CGAContextHandleEnable(CGA);
}

//

uint8_t
I8080CGAMapperContextReadXY(
    I8080CGAMapperContextPtr    cga,
    int                         x,
    int                         y
)
{
    I8080CGAMapperPrivateContext_t *CGA = (I8080CGAMapperPrivateContext_t*)cga;
    I8080Addr_t         addr = sizeof(I8080CGARegisters_t) + y * CGA->rgstrs.width + x;
    uint8_t             byte;
    
    I8080CGAContextRead(NULL, I8080AddrRangeCreate(0x0000, 0xFFFF), addr, &byte, cga);
    return byte;
}

//

void
I8080CGAMapperContextWriteXY(
    I8080CGAMapperContextPtr    cga,
    int                         x,
    int                         y,
    uint8_t                     byte
)
{
    I8080CGAMapperPrivateContext_t *CGA = (I8080CGAMapperPrivateContext_t*)cga;
    I8080Addr_t         addr = sizeof(I8080CGARegisters_t) + y * CGA->rgstrs.width + x;
    
    I8080CGAContextWrite(NULL, I8080AddrRangeCreate(0x0000, 0xFFFF), addr, byte, cga);
}
