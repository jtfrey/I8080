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

typedef uint8_t (*I8080CGAReadChCallback)(chtype ch);

static uint8_t I8080CGAReadChText(chtype ch) { return (ch & A_CHARTEXT) & 0x7f; }
static uint8_t I8080CGAReadChBWGraphics(chtype ch) { return (ch & A_REVERSE) ? 1 : 0; }
static uint8_t I8080CGAReadChClrGraphics(chtype ch) { return PAIR_NUMBER(ch); }

//

typedef void (*I8080CGAWriteChCallback)(uint8_t);

static chtype I8080CGAWriteChText(uint8_t byte) { return (chtype)(byte & 0x7f); }
static chtype I8080CGAWriteChBWGraphics(uint8_t byte) { return ((chtype)' ' | (byte ? A_REVERSE : 0)); }
static chtype I8080CGAWriteChClrGraphics(uint8_t byte) { return ((chtype)' ' | COLOR_PAIR(byte)); }

//

bool
I8080CGARead(
    I8080MemRef     mem,
    I8080Addr_t     *addr,
    uint8_t         *byte,
    const void      *context
)
{
    I8080CGAContext_t   *cga = (I8080CGAContext_t*)context;
    I8080Addr_t         cga_addr = *addr - cga->base_addr;
    
    /* Register? */
    if ( cga_addr < sizeof(I8080CGARegisters_t) ) {
        /* This is straightforward enough:  all registers are 8-bit
         * so we can just treat the address offset as an index into
         * the registers as an array of bytes.  Not quite so simple
         * for write!
         */
        *byte = cga->rgstrs.R[cga_addr];
    } else {
        /* Map the address offset to the screen element array: */
        cga_addr -= sizeof(I8080CGARegisters_t);
        if ( cga_addr < (cga->rgstrs.width * cga->rgstrs.height) ) {
            /* Turn the address into the (x, y): */
            int         y = cga_addr / cga->rgstrs.width,
                        x = cga_addr % cga->rgstrs.width;
            *byte = ((I8080CGAReadChCallback)cga->rch)(mvwinch(cga->wndw, y, x));
            return true;
        }
    }
    return false;
}

//

bool
I8080CGAWrite(
    I8080MemRef     mem,
    I8080Addr_t     *addr,
    uint8_t         byte,
    const void      *context
)
{
    I8080CGAContext_t   *cga = (I8080CGAContext_t*)context;
    I8080Addr_t         cga_addr = *addr - cga->base_addr;
    
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
            
            case kI8080CGARegisterRedraw:
                byte = byte ? 1 : 0;
                if ( cga->rgstrs.redraw != byte ) {
                    if ( (cga->rgstrs.redraw = byte) ) {
                        wnoutrefresh(cga->wndw);
                        doupdate();
                    } else {
                        redrawwin(cga->wndw);
                    }
                }
                break;
            
            case kI8080CGARegisterMode:
                if ( byte != cga->rgstrs.mode ) {
                    unsigned        mode_bit = 1 << byte;
                    
                    if ( cga->rgstrs.supmodes & mode_bit ) {
                        switch ( byte ) {
                            case kI8080CGAModeText:
                                wbkgd(cga->wndw, 0);
                                cga->rch = I8080CGAReadChText;
                                cga->wch = I8080CGAWriteChText;
                                break;
                            case kI8080CGAModeBWGraphics:
                                wbkgd(cga->wndw, (chtype)' ');
                                cga->rch = I8080CGAReadChBWGraphics;
                                cga->wch = I8080CGAWriteChBWGraphics;
                                break;
                            case kI8080CGAModeClrGraphics:
                                wbkgd(cga->wndw, (chtype)' ' | COLOR_PAIR(0));
                                cga->rch = I8080CGAReadChClrGraphics;
                                cga->wch = I8080CGAWriteChClrGraphics;
                                break;
                        }
                        wrefresh(cga->wndw);
                        cga->rgstrs.mode = byte;
                    }
                }
                break;
                
            case kI8080CGARegisterOp:
                if ( (cga->rgstrs.op & kI8080CGAOpFill) == kI8080CGAOpFill ) {
                    chtype          row[cga->rgstrs.width], ch = ((I8080CGAReadChCallback)cga->wch)(cga->rgstrs.op & 0x7f);
                    int             y;
                    
                    /* Fill the row: */
                    for ( y = 0; y < cga->rgstrs.width; y++ ) row[y] = ch;
                    
                    /* Fill the screen: */
                    if ( cga->rgstrs.redraw ) {
                        wnoutrefresh(cga->wndw);
                        doupdate();
                    }
                    for ( y = 0; y < cga->rgstrs.height; y++ ) mvwaddchnstr(cga->wndw, y, 0, row, cga->rgstrs.width);
                    if ( cga->rgstrs.redraw ) {
                        redrawwin(cga->wndw);
                    }
                } else {
                    switch ( cga->rgstrs.op ) {
                        case kI8080CGAOpClear:
                            wclear(cga->wndw);
                            if ( cga->rgstrs.redraw ) wrefresh(cga->wndw);
                            break;
                        case kI8080CGAOpClearRow: {
                            int         x;
                            
                            if ( cga->rgstrs.redraw ) {
                                wnoutrefresh(cga->wndw);
                                doupdate();
                            }
                            mvwaddch(cga->wndw, cga->rgstrs.y, 0, 0);
                            for ( x = 1; x < cga->rgstrs.width; x++ ) waddch(cga->wndw, 0);
                            if ( cga->rgstrs.redraw ) {
                                redrawwin(cga->wndw);
                            }
                            break;
                        }
                        case kI8080CGAOpClearCol: {
                            int         y;
                            
                            if ( cga->rgstrs.redraw ) {
                                wnoutrefresh(cga->wndw);
                                doupdate();
                            }
                            for ( y = 0; y < cga->rgstrs.height; y++ )
                                mvwaddch(cga->wndw, y, cga->rgstrs.x, 0);
                            if ( cga->rgstrs.redraw ) {
                                redrawwin(cga->wndw);
                            }
                            break;
                        }
                        case kI8080CGAOpWriteNChars: {
                            if ( cga->rgstrs.i > 0 ) {
                                I8080Addr_t addr = ((I8080Addr_t)cga->rgstrs.u16hi << 8) | cga->rgstrs.u16lo;
                                uint8_t     b = I8080MemRead(mem, addr++);
                                
                                if ( cga->rgstrs.redraw ) {
                                    wnoutrefresh(cga->wndw);
                                    doupdate();
                                }
                                mvwaddch(cga->wndw, cga->rgstrs.y, cga->rgstrs.x, ((I8080CGAReadChCallback)cga->wch)(b));
                                while ( --cga->rgstrs.i ) {
                                    b = I8080MemRead(mem, addr++);
                                    waddch(cga->wndw, ((I8080CGAReadChCallback)cga->wch)(b));
                                }
                                if ( cga->rgstrs.redraw ) {
                                    redrawwin(cga->wndw);
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
                                if ( cga->rgstrs.redraw ) {
                                    wnoutrefresh(cga->wndw);
                                    doupdate();
                                }
                                mvwaddch(cga->wndw, cga->rgstrs.y, cga->rgstrs.x, ((I8080CGAReadChCallback)cga->wch)(b));
                                while ( (b = I8080MemRead(mem, addr++)) ) {
                                    waddch(cga->wndw, ((I8080CGAReadChCallback)cga->wch)(b));
                                }
                                if ( cga->rgstrs.redraw ) {
                                    redrawwin(cga->wndw);
                                }
                            }
                            cga->rgstrs.u16hi = (addr & 0xFF00) >> 8,
                            cga->rgstrs.u16lo = (addr & 0xFF);
                            break;
                        }
                        case kI8080CGAOpGetColorRGB: {
                            I8080CGAContextGetColor(cga, cga->rgstrs.i, &cga->rgstrs.r, &cga->rgstrs.g, &cga->rgstrs.b);
                            break;
                        }
                        case kI8080CGAOpSetColorRGB: {
                            I8080CGAContextSetColor(cga, cga->rgstrs.i, cga->rgstrs.r, cga->rgstrs.g, cga->rgstrs.b);
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
            mvwaddch(cga->wndw, y, x, ((I8080CGAReadChCallback)cga->wch)(byte));
            return true;
        }
    }
    return false;
}

//

void
I8080CGACleanup(
    I8080MemRef         mem,
    const void          *context
)
{
    I8080CGAContext_t   *cga = (I8080CGAContext_t*)context;
    int                 c = 1;
    short               *s = &cga->saved_colors[0],
                        *s_end = s + sizeof(cga->saved_colors);
                    
    while ( s < s_end ) {
        if ( *s ) init_color(c, *(s + 1), *(s + 2), *(s + 3));
        s += 4, c++;
    }
    if ( ! cga->rgstrs.redraw ) {
        wnoutrefresh(cga->wndw);
        doupdate();
    }
    delwin(cga->wndw);
    memset(&cga->saved_colors[0], 0, sizeof(cga->saved_colors));
    memset(&cga->rgstrs, 0, sizeof(cga->rgstrs));
    cga->wndw = NULL;
    cga->rch = cga->wch = NULL;
}

//

const I8080MemCallbacks __I8080CGACallbacks = {
            .read = I8080CGARead,
            .write = I8080CGAWrite,
            .cleanup = I8080CGACleanup
        };
const I8080MemCallbacks *I8080CGACallbacks = &__I8080CGACallbacks;

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

I8080CGAContextPtr
I8080CGAContextCreate(
    I8080Addr_t         base_addr,
    I8080CGAMode_t      mode,
    WINDOW              *wndw
)
{
    I8080CGAContextPtr  context = NULL;
    bool                does_support_color = has_colors();
    I8080CGAMode_t      supmodes = does_support_color ? kI8080CGAModeAllModesMask : kI8080CGAModeBasicModesMask;
    
    if ( ((1 << mode) & supmodes) == 0 ) {
        ERROR("Desired display mode %d is not supported", mode);
    } else if ( (context = (I8080CGAContextPtr)calloc(1, sizeof(I8080CGAContext_t))) ) {
        
        context->base_addr = base_addr;
        context->wndw = wndw;
        
        // Fill-in dimensions:
        getmaxyx(wndw, context->rgstrs.height, context->rgstrs.width);
        
        // What modes are supported:
        context->rgstrs.supmodes = supmodes;
        
        // How many colors are supported?
        context->rgstrs.ncolors = does_support_color ? ((COLORS < 254) ? COLORS : 254) : 0;
        
        // Initially enable redraw mode:
        context->rgstrs.redraw = 1;
        
        // Mode setup:
        switch ( (context->rgstrs.mode = mode) ) {
            case kI8080CGAModeText:
                wbkgd(wndw, 0);
                context->rch = I8080CGAReadChText;
                context->wch = I8080CGAWriteChText;
                break;
            case kI8080CGAModeBWGraphics:
                wbkgd(wndw, (chtype)' ');
                context->rch = I8080CGAReadChBWGraphics;
                context->wch = I8080CGAWriteChBWGraphics;
                break;
            case kI8080CGAModeClrGraphics:
                wbkgd(wndw, (chtype)' ' | COLOR_PAIR(0));
                context->rch = I8080CGAReadChClrGraphics;
                context->wch = I8080CGAWriteChClrGraphics;
                break;
            default:
                break;
        }
        wclear(wndw);
        wrefresh(wndw);
    }
    return context;
}

//

void
I8080CGAContextGetColor(
    I8080CGAContextPtr  cga,
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
I8080CGAContextSetColor(
    I8080CGAContextPtr  cga,
    int                 color,
    uint8_t             r,
    uint8_t             g,
    uint8_t             b
)
{
    short               sr = 1000 * ((double)r / 255.0),
                        sg = 1000 * ((double)g / 255.0),
                        sb = 1000 * ((double)b / 255.0);
    short               *s = &cga->saved_colors[4 * (color - 1)];
    
    if ( ! *s ) {
        *s = 1;
        color_content(color, s + 1, s + 2, s + 3);
    }
    init_color(color, sr, sg, sb);
    init_pair(color, color, color);
}

//

void
I8080CGAContextRestoreColors(
    I8080CGAContextPtr  cga
)
{
    int                 c = 1;
    short               *s = &cga->saved_colors[0],
                        *s_end = s + sizeof(cga->saved_colors);
                        
    while ( s < s_end ) {
        if ( *s ) init_color(c, *(s + 1), *(s + 2), *(s + 3));
        s += 4, c++;
    }
    memset(cga->saved_colors, 0, sizeof(cga->saved_colors));
}

//

void
I8080CGAContextLoadColorPalette(
    I8080CGAContextPtr  cga,
    I8080CGAPalettePtr  palette
)
{
    const I8080CGAColor_t   *colors = palette->colors;
    int                     color;
    
    I8080CGAContextRestoreColors(cga);
    for ( color = 0; color < palette->n_colors; colors++ )
        I8080CGAContextSetColor(cga, ++color, colors->r, colors->g, colors->b);
}

//

uint8_t
I8080CGAContextReadXY(
    I8080CGAContextPtr  cga,
    int                 x,
    int                 y
)
{
    I8080Addr_t         addr = cga->base_addr + sizeof(I8080CGARegisters_t) + x + y * cga->rgstrs.width;
    uint8_t             byte;
    
    return I8080CGARead(NULL, &addr, &byte, cga);
}

//

void
I8080CGAContextWriteXY(
    I8080CGAContextPtr  cga,
    int                 x,
    int                 y,
    uint8_t             byte
)
{
    I8080Addr_t         addr = cga->base_addr + sizeof(I8080CGARegisters_t) + x + y * cga->rgstrs.width;
    
    I8080CGAWrite(NULL, &addr, byte, cga);
}
