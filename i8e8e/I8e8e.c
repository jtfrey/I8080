/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * CLI 8080 emulator.
 *
 */

#include "I8080System.h"
#include "I8080Logging.h"
#include "I8e8eMemObj.h"
#include "I8e8eBSMObj.h"
#include "I8e8eFileDeviceObj.h"
#include "I8080CGA.h"
#ifdef I8E8E_HAVE_PPU
#   include "I8080PPU.h"
#   include "I8080DPad.h"
#endif
#ifdef I8E8E_HAVE_APU
#   include "I8080APU.h"
#endif
#include "I8080Timer.h"
#include <fcntl.h>
#include <signal.h>
#include <getopt.h>


#ifndef I8E8E_CGA_HARD_PAGELIMIT
#   define I8E8E_CGA_HARD_PAGELIMIT 0x40
#endif
#ifndef I8E8E_CGA_SOFT_PAGELIMIT
#   define I8E8E_CGA_SOFT_PAGELIMIT 0x20
#endif
#if I8E8E_CGA_SOFT_PAGELIMIT > I8E8E_CGA_HARD_PAGELIMIT
#   undef I8E8E_CGA_HARD_PAGELIMIT
#   undef I8E8E_CGA_SOFT_PAGELIMIT
#   error I8E8E_CGA_SOFT_PAGELIMIT exceeds I8E8E_CGA_HARD_PAGELIMIT
#endif
const I8080Addr_t I8e8eCGAHardPageLimit = I8E8E_CGA_HARD_PAGELIMIT;
const I8080Addr_t I8e8eCGASoftPageLimit = I8E8E_CGA_SOFT_PAGELIMIT;


enum {
    kI8e8eOptAlternatePC        = 0x1000,
    kI8e8eOptAlternateSP        = 0x1001
};

static struct option i8e8e_options[] = {
        { "help",           no_argument,        0,  'h' },
        { "verbose",        no_argument,        0,  'v' },
        { "quiet",          no_argument,        0,  'q' },
        { "logfile",        required_argument,  0,  'l' },
        { "disasm",         optional_argument,  0,  'D' },
        { "load",           required_argument,  0,  'L' },
        { "rom",            required_argument,  0,  'R' },
#ifdef HAS_MMAP
        { "rom-map-mode",   required_argument,  0,  'm' },
#endif
        { "unmapped",       required_argument,  0,  'U' },
        { "bsm",            required_argument,  0,  'b' },
        { "core",           required_argument,  0,  'C' },
        { "core-kind",      required_argument,  0,  'k' },
        { "pc",             required_argument,  0,  'P' },
        { "PC",             required_argument,  0,  kI8e8eOptAlternatePC },
        { "sp",             required_argument,  0,  'S' },
        { "SP",             required_argument,  0,  kI8e8eOptAlternateSP },
        { "2mhz",           no_argument,        0,  '2' },
        { "accel-instrs",   no_argument,        0,  'A' },
        { "file-device",    required_argument,  0,  'f' },
        { "tty",            required_argument,  0,  't' },
        { "timers",         required_argument,  0,  'T' },
        { "cga",            required_argument,  0,  'c' },
#ifdef I8E8E_HAVE_PPU
        { "ppu",            required_argument,  0,  'g' },
        { "dpad",           optional_argument,  0,  'd' },
        { "list-palettes",  no_argument,        0,  'p' },
#endif
#ifdef I8E8E_HAVE_APU
        { "apu",            required_argument,  0,  'a' },
#endif
        { NULL,             0,                  0,   0  }
    };

static const char *i8e8e_options_str = 
        "hvql:D:L:R:"
#ifdef HAS_MMAP
        "m:"
#endif
        "U:b:C:k:P:S:2Af:t:T:c:"
#ifdef I8E8E_HAVE_PPU
        "g:d:p"
#endif
#ifdef I8E8E_HAVE_APU
        "a:"
#endif
        "";

void
usage(
    const char  *exe
)
{
    fprintf(stderr,
            "usage:\n\n"
            "    %s {options}\n\n"
            "  options:\n\n"
            "    -h/--help                      show this help\n"
            "    -v/--verbose                   increase logging level\n"
            "    -q/--quiet                     decrease logging level\n"
            "    -l/--logfile <log-dest>        redirect logging output to the destination;\n"
            "                                   new lines are appended to existing data\n"
            "    -D/--disasm                    write running disassembly of the program\n"
            "                                   to stderr\n"
            "    --disasm=<filename>            write running disassembly of the program\n"
            "                                   to the named file provided; the file is \n"
            "                                   overwritten and not appended\n"
            "    -L/--load <bytes-in>           load bytes from a file into the system\n"
            "                                   system memory of the emulator\n"
            "    -R/--rom <bytes-in>            map bytes from from a file as a ROM\n"
            "                                   image in the emulator's memory space\n"
#ifdef HAS_MMAP
            "    -m/--rom-map-mode <rom-mode>   select the ROM mapping mode\n"
#endif
            "    -U/--unmapped <unmapped-seg>   the given range of memory addresses are\n"
            "                                   made unwritable and a static value is\n"
            "                                   returned on read\n"
            "    -b/--bsm <bsm-spec>            add a bank-swapped memory expander in the\n"
            "                                   specified page range\n"
            "    -C/--core <filename>           when the emulator exits, dump the system memory\n"
            "                                   to <filename>\n"
            "    -k/--core-kind <core-kind>     select which kind of core dump to make; default\n"
            "                                   is binary\n"
            "    -P/--pc/--PC <addr>            on start, set the program counter (PC)\n"
            "                                   to this address (default: $0000)\n"
            "    -S/--sp/--SP <addr>            on start, set the stack pointer (SP)\n"
            "                                   to this address (default: $0000)\n"
            "    -2/--2mhz                      limit the instruction dispatch rate to\n"
            "                                   a clock frequency of 2 MHz; without this\n"
            "                                   flag instructions are dispatched as fast\n"
            "                                   as possible\n"
            "    -A/--accel-instrs              use the monolithic, accelerated instruction\n"
            "                                   handler\n"
            "    -r/--rl-rsrc-report            print a summary of resource usage to stdout\n"
            "                                   after the system transitions out of the running\n"
            "                                   state; otherwise the report gets logged at the\n"
            "                                   INFO level\n"
            "    -f/--file-device <file-dev>    connect a file to the device bus of the\n"
            "                                   emulator\n"
            "    -t/--tty <tty-options>         configure TTY options\n"
            "    -T/--timers <timers-options>   configure realtime timers\n"
            "    -c/--cga <cga-options>         configure a Curses Graphics Adapter...\n"
#ifdef I8E8E_HAVE_PPU
            "    -g/--ppu <ppu-options>         ...or configure a Picture Processing Unit\n"
            "    -d/--dpad                      attach a D-pad controller input device on\n"
            "                                   the first available input channel number\n"
            "    --dpad=<dev-id>                attach a D-pad on a specific input channel\n"
            "                                   number\n"
            "    -p/--list-palettes             list the available CGA color palettes\n"
            "                                   then exit\n"
#endif
#ifdef I8E8E_HAVE_APU
            "    -a/--apu <apu-options>         configure an Audio Processing Unit\n"
#endif
            "\n"
            "    <log-dest> = <filepath>{:<buffer>}\n\n"
            "        The file at <filepath> will have logged information appended to it.\n"
            "        By default, <buffer>=4096 buffering is used to minimize the impact on\n"
            "        emulator performance.  If increased flushing of log data is preferable,\n"
            "        use a lower value OR <buffer>=line (or -1) for line buffering or\n"
            "        <buffer>=none (or 0) for no buffering at all!\n"
            "\n"
            "    <addr> = ( $XXXX | 0xXXXX | 0N… | N… )\n\n"
            "        The emulator features a 64 KiB address space.  Addresses can be\n"
            "        expressed in dollar-sign notation or with a '0x' prefix for\n"
            "        hexedecimal values, or as an octal or decimal value.\n"
            "\n"
            "    <page> = $XX | 0xXX | 0NNN | N\n\n"
            "        The 64 KiB address space is arranged in pages of 256 bytes; there are\n"
            "        256 such pages from $00…$FF.\n"
            "\n"
            "    <offset>, <len>, <byte> = { $X… | 0xX… | 0N… | N… }\n\n"
            "        An <offset>, <len>, or <byte> are a hexadecimal, octal, or decimal value\n"
            "\n"
            "    <bytes-in> = <filepath>{:<offset>}{#<len>}@<addr>{-<addr2>}\n\n"
            "        Bytes are read from <filepath> starting at <offset> (or the\n"
            "        beginnining of the file if <offset> is not provided) and inserted\n"
            "        into the system memory of the emulator starting at <addr>.  Up to\n"
            "        <len> bytes are inserted (or the full length of the file after\n"
            "        <offset> if <len> is omitted) with the operation wrapping back around\n"
            "        the address space as necessary.  The optional <addr2> provides an end\n"
            "        address that may be longer/shorter than the length read from the file.\n"
            "\n"
#ifdef HAS_MMAP
            "    <rom-mode> = ( alloc | mmap )\n\n"
            "        When ROM images are created, the file contents can either be loaded into\n"
            "        an allocated buffer in the emulator's memory space or the kernel can map\n"
            "        the byte range indirectly via mmap()\n"
            "\n"
#endif
            "    <unmapped-seg> = <addr>-<addr2>:<byte>\n\n"
            "        An unmapped segment is created for which <byte> will be returned on\n"
            "        read and writes will be ignored.  The address range starts at the first\n"
            "        <addr> and goes through and including <addr2>\n"
            "\n"
            "    <bsm-spec> = <page>{+<page>}{#<dev-id>}:<file>{+W|+R}{:<file>{+W|+R}…}\n"
            "        A bank-swapped memory expander combines multiple data sets existing in the\n"
            "        same page range of system memory; an i/o device channel provides the user\n"
            "        program the ability to select which bank of data the page range accesses.\n"
            "        The leading <page> indicates at which page the BSM should be mapped, and\n"
            "        {+<page>} is how many pages it should occupy (defaulting to 1 page).  If only\n"
            "        one <file> is provided, its entire contents will be broken into #<page>\n"
            "        selectable banks.  If multiple <file> elements are provided, then each file\n"
            "        will act as a selectable bank, with at most #<page> pages of data usable\n"
            "        inside the emulator.  The {+W|+R} flag is an optional suffix on the <file>\n"
            "        that indicates whether or not the bank should be RAM or ROM (ROM is default).\n"
            "        Note that the selected <rom-mode> will affect how the file data are moved into\n"
            "        memory (as allocated buffers or mmap'ed).\n\n"
            "        The optional #<dev-id> indicates on what input/output channels the BSM's\n"
            "        bank select should be connected; if not provided, the device bus will assign\n"
            "        the next-available device ids at registration.\n"
            "\n"
            "    <file-dev> = <filepath>:<i|o|io>{#<dev-id>}{@<mode>}\n\n"
            "        A file at the given <filepath> is connected to the device bus of\n"
            "        the emulator.  If <dev-id> is given, that specific device id will\n"
            "        be used (if available); otherwise, the device bus will choose the\n"
            "        next-available device id.  The file will be opened in append+read\n"
            "        mode by default (a+); the modes recognized by fopen() are\n"
            "        permissible.  The 'i' vs. 'o' vs. 'io' indicate which device type\n"
            "        the file should be associated with:  input, output, or combined\n"
            "        input and output.\n\n"
            "        By default input device 0 is connected to stdin; output device 0 is\n"
            "        connected to stdout; and output device 255 is connected to stderr.\n"
            "\n"
            "    <tty-options> = <tty-option>{,<tty-option>{,...}}\n"
            "        Valid <tty-option> flags are:\n\n"
            "            o-clear-on-reset   when the emulator is powered on/reset clear\n"
            "                               stdout\n"
            "            o-all              all output options enabled\n"
            "            i-single-char      enabled single-character input mode on stdin\n"
            "            i-disable-echo     disable echoing of typed characters on stdin\n"
            "            i-all              all input options enabled\n"
            "            all                all input and output options enabled\n"
            "\n"
            "    <timers-options> = <addr>\n"
            "        Create a realtime timer device that will be mapped at memory location\n"
            "        <addr>.  The first 16-bytes of the adapter are read-only date and time\n"
            "        registers.  The remaining registers are used to configure and manage\n"
            "        timers that generate an interrupt on expiration.\n"
            "\n"
            "    <cga-options> = <text|bw|color>@<addr>{#<w>x<h>{/<x>,<y>}}{:<palette-id>}\n"
            "        Create a Curses Graphics Adapter that will be mapped at memory location\n"
            "        <addr>.  The first 16-bytes of the adapter function as 8-bit registers\n"
            "        for user code to interact with the device (see the programming manual\n"
            "        for more information).  The initial mode is mandatory.  The palette\n"
            "        defaults to a 16-color, 4-level RGB palette from https://lospec.com/.\n"
            "        By default the full curses screen will be used; this can be overridden\n"
            "        by providing a <w> and <h> (width and height) and optionally the origin\n"
            "        of the resulting window, (<x>, <y>).\n"
            "\n"
#ifdef I8E8E_HAVE_PPU
            "    <ppu-options> = <addr>{:<palette-id>}\n"
            "        Create a curses-based Picture Processing Unit that will be mapped at\n"
            "        memory location <addr>.  The PPU and CGA memory mappers are mutually\n"
            "        exclusive.  The first 16-bytes of the PPU function as 8-bit registers\n"
            "        for control and feedback.  The remainder of the 1 KiB of address space\n"
            "        is split between palette, tile, and sprite tables and two tile maps.\n"
            "        See the programmer's manual for more information.  This library reuses\n"
            "        the CGA color palette functionality, so the rendering of the tiles and\n"
            "        sprites usually requires a specific <palette-id> be used.  The default\n"
            "        is the kI8080CGAPaletteNES2C02 palette.\n"
            "\n"
#endif
#ifdef I8E8E_HAVE_APU
            "    <apu-options> = <addr>\n"
            "        Create an Audio Processing Unit that will be mapped at memory location\n"
            "        <addr>.  Bytes starting at <addr> function as control registers for the\n"
            "        pairs of pulse and triangle channels as well as the noise channel.  See the\n"
            "        libi8008APU documentation for an in-depth discussion of the available\n"
            "        registers.\n"
            "\n"
#endif
            "    <core-kind> = text | binary\n"
            "        The kind of core dump to generate.  A text-based dump abbreviates ranges\n"
            "        of zeroes and omits mapped pages, whereas a binary dump will be the full\n"
            "        64 KiB of system memory.\n"
            "\n"
            "  environment variables:\n\n"
            "    I8E8E_VERBOSITY        the initial logging level; any abbreviation of\n\n"
            "                             critical, error, warning, info, debug\n\n"
            "                           is accepted as well as integer values\n"
            "    I8E8E_PC               the initial value of the program counter (PC)\n"
            "    I8E8E_SP               the initial value of the stack pointer (SP)\n"
            "    I8E8E_ROM_MAPPING_MODE the initial value of the ROM mapping mode\n"
            "\nversion %s\n",
            exe,
            I8080VersionString
        );
}

//

#ifdef I8E8E_HAVE_PPU

typedef struct {
    I8080Addr_t                 base_addr;
    I8080CGAPaletteId_t         palette_id;
    I8080PPUMapperContextPtr    context;
} I8e8ePPU_t;

bool
I8e8ePPUParse(
    const char      *in_str,
    I8e8ePPU_t      *out_ppu
)
{
    const char      *in_str_orig = in_str;
    const char      *s_end;
    
    // Parse the address string:
    if ( ! I8080AddrParse(in_str, &out_ppu->base_addr, &s_end) ) {
        ERROR("Invalid PPU mapping address: %s", in_str_orig);
        return false;
    }
    
    // The next character must be a NUL or in the options set "#/:"
    out_ppu->palette_id = kI8080CGAPaletteNES2C02;
    if ( *s_end && ((*s_end != ':') || ! I8080CGAPaletteIdParse(++s_end, &out_ppu->palette_id, &s_end)) ) {
        ERROR("Invalid PPU palette id: %s", in_str_orig);
        ERROR("                        %*s^", (s_end - in_str_orig), "");
        return false;
    }
    if ( *s_end ) {
        ERROR("Invalid PPU mapping: %s", in_str_orig);
        ERROR("                     %*s^", (s_end - in_str_orig), "");
        return false;
    }
    return true;
}

#endif

//

#ifdef I8E8E_HAVE_APU

typedef struct {
    I8080Addr_t                 base_addr;
    I8080APUMapperContextPtr    context;
} I8e8eAPU_t;

bool
I8e8eAPUParse(
    const char      *in_str,
    I8e8eAPU_t      *out_apu
)
{
    const char      *in_str_orig = in_str;
    const char      *s_end;
    
    // Parse the address string:
    if ( ! I8080AddrParse(in_str, &out_apu->base_addr, &s_end) ) {
        ERROR("Invalid APU mapping address: %s", in_str_orig);
        return false;
    }
    
    if ( *s_end ) {
        ERROR("Invalid APU mapping: %s", in_str_orig);
        ERROR("                     %*s^", (s_end - in_str_orig), "");
        return false;
    }
    return true;
}

#endif

//

typedef struct {
    I8080Addr_t                 base_addr;
    I8080CGAMode_t              mode;
    I8080CGAPaletteId_t         palette_id;
    bool                        have_dims;
    bool                        have_coords;
    int                         h, w, y, x;
    I8080CGAMapperContextPtr    context;
} I8e8eCGA_t;

bool
I8e8eCGAParse(
    const char      *in_str,
    I8e8eCGA_t      *out_cga
)
{
    const char      *in_str_orig = in_str;
    const char      *s_end;
    
    out_cga->h = out_cga->w = out_cga->x = out_cga->y = 0;
    
    // Skip ahead to the '@' delimiter:
    s_end = in_str;
    while ( *s_end && (*s_end != '@') ) s_end++;
    if ( *s_end != '@' ) {
        ERROR("Invalid CGA specification: %s", in_str_orig);
        return false;
    }
    
    // What mode?
    if ( strncasecmp(in_str, "text", (s_end - in_str)) == 0 ) {
        out_cga->mode = kI8080CGAModeText;
    }
    else if ( strncasecmp(in_str, "bw", (s_end - in_str)) == 0 ) {
        out_cga->mode = kI8080CGAModeBWGraphics;
    }
    else if ( strncasecmp(in_str, "color", (s_end - in_str)) == 0 ) {
        out_cga->mode = kI8080CGAModeClrGraphics;
    }
    else {
        ERROR("Invalid CGA mode: %s", in_str_orig);
        return false;
    }
    in_str = s_end;
    
    // Find end of address string:
     ++in_str;
    if ( ! I8080AddrParse(in_str, &out_cga->base_addr, &s_end) ) {
        ERROR("Invalid CGA mapping address: %s", in_str_orig);
        return false;
    }
    
    // The next character must be a NUL or in the options set "#/:"
    out_cga->palette_id = kI8080CGAPaletteIdDefault;
    while ( *s_end && strchr("#/:", *s_end) ) {
        switch ( *s_end ) {
            case ':': {
                // Parse the palette id:
                if ( ! I8080CGAPaletteIdParse(++s_end, &out_cga->palette_id, &s_end) ) {
                    ERROR("Invalid CGA palette id: %s", in_str_orig);
                    ERROR("                        %*s^", (s_end - in_str_orig), "");
                    return false;
                }
                break;
            }
            case '#': {
                int     nchars;
                
                if ( sscanf(++s_end, "%dx%d%n", &out_cga->w, &out_cga->h, &nchars) != 2 ) {
                    ERROR("Invalid CGA mapping: %s", in_str_orig);
                    ERROR("                     %*s^", (s_end - in_str_orig), "");
                    return false;
                }
                else if ( out_cga->h < 2 ) {
                    ERROR("Invalid CGA mapping, height is < 2: %s", in_str_orig);
                    return false;
                }
                else if ( out_cga->w < 2 ) {
                    ERROR("Invalid CGA mapping, width is < 2: %s", in_str_orig);
                    return false;
                }
                out_cga->have_dims = true;
                s_end += nchars;
                break;
            }
            case '/': {
                int     nchars;
                
                if ( sscanf(++s_end, "%d,%d%n", &out_cga->x, &out_cga->y, &nchars) != 2 ) {
                    ERROR("Invalid CGA mapping: %s", in_str_orig);
                    ERROR("                     %*s^", (s_end - in_str_orig));
                    return false;
                }
                out_cga->have_coords = true;
                s_end += nchars;
                break;
            }
        }
    }
    if ( *s_end ) {
        ERROR("Invalid CGA mapping: %s", in_str_orig);
        ERROR("                     %*s^", (s_end - in_str_orig), "");
        return false;
    }
    return true;
}

//

static struct I8080DeviceTTYOptsPair {
    const char  *tty_option;
    int         flag;
} I8080DeviceTTYOptsMap[] = {
    { "o-clear-on-reset",       (int)kI8080DeviceStdOutputOptsClearScreenOnReset    },
    { "i-single-char",          (int)kI8080DeviceStdInputOptsSetNonCanonicalMode    },
    { "i-disable-echo",         (int)kI8080DeviceStdInputOptsDisableEcho,           },
    { NULL,                     0                                                   } };

bool
I8080DeviceTTYOptsParse(
    const char                  *in_str,
    I8080DeviceStdInputOpts_t   *in_opts,
    I8080DeviceStdOutputOpts_t  *out_opts
)
{
    const char                  *in_str_orig = in_str;
    
    while ( *in_str ) {
        struct I8080DeviceTTYOptsPair   *pairs = I8080DeviceTTYOptsMap;
        const char                      *token_end = in_str;
        
        while ( *token_end && ! strchr(",:;+ ", *token_end) ) token_end++;
        if ( token_end > in_str ) {
            // Between in_str and token_end is the string:
            int         token_len = token_end - in_str;
            
            if ( strncasecmp("o-all", in_str, token_len) == 0 ) {
                *out_opts = kI8080DeviceStdOutputOptsAll;
            }
            else if ( strncasecmp("i-all", in_str, token_len) == 0 ) {
                *in_opts = kI8080DeviceStdInputOptsAll;
            }
            else if ( strncasecmp("all", in_str, token_len) == 0 ) {
                *in_opts = kI8080DeviceStdInputOptsAll;
                *out_opts = kI8080DeviceStdOutputOptsAll;
            }
            else {
                while ( pairs->tty_option ) {
                    if ( strncasecmp(pairs->tty_option, in_str, token_len) == 0 ) {
                        switch ( *in_str ) {
                            case 'o':
                            case 'O':
                                *out_opts |= pairs->flag;
                                break;
                            case 'i':
                            case 'I':
                                *in_opts |= pairs->flag;
                                break;
                        }
                        break;
                    }
                    pairs++;
                }
                if ( ! pairs->tty_option ) {
                    ERROR("Invalid TTY option:  %s", in_str_orig);
                    ERROR("                    %*s^", (in_str - in_str_orig), "");
                    return false;
                }
            }
        }
        while ( *token_end && strchr(",:;+ ", *token_end) ) token_end++;
        in_str = token_end;
    }
    return true;
}

//

volatile I8080SystemPtr sys8080 = NULL;

void handle_sigint(int sig) {
    I8080SystemBreak(sys8080);
}

//

int
main(
    int             argc,
    char            **argv,
    char            **envp
)
{
    int                         opt = 0, opt_index = 0;
    I8080SystemOpts_t           system_opts = 0;
    I8080DeviceStdInputOpts_t   tty_in_opts = 0;
    I8080DeviceStdOutputOpts_t  tty_out_opts = 0;
    I8080Addr_t                 SP = 0x0000, PC=0x0000;
    I8e8eCGA_t                  cga;
    bool                        have_cga = false;
#ifdef I8E8E_HAVE_PPU
    I8e8ePPU_t                  ppu;
    bool                        have_ppu = false;
    I8080DevBusRegisterId       dpad_dev_id = kI8080DevBusRegisterIdNextAvail;
    bool                        have_dpad = false;
#endif
#ifdef I8E8E_HAVE_APU
    I8e8eAPU_t                  apu;
    bool                        have_apu = false;
#endif
    I8e8eMemObj_t               *memobjs = NULL, *memobjs_tail = NULL;
    I8e8eROMMappingMode_t       rom_mapmode = kI8e8eROMMappingModeAlloc;
    I8e8eBSMObj_t               *bsmobjs = NULL, *bsmobjs_tail = NULL;
    I8e8eFileDeviceObj_t        *filedevobjs = NULL, *filedevobjs_tail = NULL;
    I8080Addr_t                 timer_addr;
    I8080TimerContextPtr        timer = NULL;
    bool                        have_timer = false;
    I8080MemCoreDumpKind_t      core_kind = kI8080MemCoreDumpKindBinary;
    const char                  *want_core_dump = NULL;
    bool                        want_disasm = false;
    const char                  *disasm_file = NULL;
    char                        *str;
    
    signal(SIGINT, handle_sigint);
    
    if ( (str = getenv("I8E8E_VERBOSITY")) ) {
        I8080LoggingLevel_t     loglevel;
        
        if ( I8080LoggingLevelParse(str, &loglevel) ) {
            I8080LoggingSetMaxLevel(loglevel);
        } else {
            ERROR("Unable to parse log level from $I8E8E_VERBOSITY=%s", str);
            exit(EINVAL);
        }
    }
    if ( (str = getenv("I8E8E_PC")) ) {
        if ( ! I8080AddrParse(str, &PC, NULL) ) {
            ERROR("Unable to parse program counter (PC) from $I8E8E_PC=%s", str);
            exit(EINVAL);
        }
    }
    if ( (str = getenv("I8E8E_SP")) ) {
        if ( ! I8080AddrParse(str, &SP, NULL) ) {
            ERROR("Unable to parse stack pointer (SP) from $I8E8E_SP=%s", str);
            exit(EINVAL);
        }
    }
#ifdef HAS_MMAP
    if ( (str = getenv("I8E8E_ROM_MAPPING_MODE")) ) {
        if ( ! I8e8eROMMappingModeParse(str, &rom_mapmode) ) {
            ERROR("Unable to parse ROM mapping mode from $I8E8E_ROM_MAPPING_MODE=%s", str);
            exit(EINVAL);
        }
    }
#endif
    
    while ( (opt = getopt_long(argc, argv, i8e8e_options_str, i8e8e_options, &opt_index)) != -1 ) {
        switch ( opt ) {
            case 'h':
                usage(argv[0]);
                exit(0);
            case 'v':
                I8080LoggingLevelIncrement();
                break;
            case 'q':
                I8080LoggingLevelDecrement();
                break;
            
            case 'l': {
                char        *colon = strchr(optarg, ':');
                
                if ( colon ) {
                    I8080LoggingBufferType_t    buffer_type = kI8080LoggingBufferTypeDefault;
                    char                        *filepath = strndup(optarg, (colon - optarg));
                    colon++;
                    if ( strcasecmp(colon, "none") == 0 ) {
                        buffer_type = kI8080LoggingBufferTypeNone;
                    }
                    else if ( strcasecmp(colon, "line") == 0 ) {
                        buffer_type = kI8080LoggingBufferTypeLine;
                    }
                    else {
                        char        *endptr;
                        long        buffer_size = strtol(colon, &endptr, 0);
                        
                        if ( (endptr == colon) || (buffer_size < kI8080LoggingBufferTypeLine) ) {
                            ERROR("Invalid log buffering kind: %s", colon);
                            exit(EINVAL);
                        }
                        buffer_type = buffer_size;
                    }
                    I8080LoggingSetBuffering(buffer_type);
                    if ( ! I8080LoggingSetFile(filepath, true) ) {
                        ERROR("Unable to open file for logging: %s", filepath);
                        exit(errno);
                    }
                    free((void*)filepath);
                } else {
                    if ( ! I8080LoggingSetFile(optarg, true) ) {
                        ERROR("Unable to open file for logging: %s", optarg);
                        exit(errno);
                    }
                }
                break;
            }
            
            case 'D': {
                system_opts |= kI8080SystemOptsDisasmToFile;
                disasm_file = optarg;
                break;
            }
            
            case 'L': {
                I8e8eMemObj_t  *memobj = I8e8eMemObjParse(kI8e8eMemObjTypeData, optarg);
                
                if ( ! memobj ) exit(EINVAL);
                if ( memobjs_tail ) {
                    memobjs_tail->link = memobj;
                    memobjs_tail = memobj;
                } else {
                    memobjs = memobjs_tail = memobj;
                }
                break;
            }
            
            case 'R': {
                I8e8eMemObj_t  *memobj = I8e8eMemObjParse(kI8e8eMemObjTypeROM, optarg);
                
                if ( ! memobj ) exit(EINVAL);
                if ( memobjs_tail ) {
                    memobjs_tail->link = memobj;
                    memobjs_tail = memobj;
                } else {
                    memobjs = memobjs_tail = memobj;
                }
                break;
            }
            
#ifdef HAS_MMAP
            case 'm': {
                if ( ! I8e8eROMMappingModeParse(optarg, &rom_mapmode) ) {
                    ERROR("Unable to parse ROM mapping mode from $I8E8E_ROM_MAPPING_MODE=%s", str);
                    exit(EINVAL);
                }
                break;
            }
#endif

            case 'U': {
                I8e8eMemObj_t  *memobj = I8e8eMemObjParse(kI8e8eMemObjTypeUnmapped, optarg);
                
                if ( ! memobj ) exit(EINVAL);
                if ( memobjs_tail ) {
                    memobjs_tail->link = memobj;
                    memobjs_tail = memobj;
                } else {
                    memobjs = memobjs_tail = memobj;
                }
                break;
            }
            
            case 'b': {
                I8e8eBSMObj_t  *bsmobj = I8e8eBSMObjParse(rom_mapmode, optarg);
                
                if ( ! bsmobj ) exit(EINVAL);
                if ( bsmobjs_tail ) {
                    bsmobjs_tail->link = bsmobj;
                    bsmobjs_tail = bsmobj;
                } else {
                    bsmobjs = bsmobjs_tail = bsmobj;
                }
                break;
            }
            
            case 'C':
                want_core_dump = optarg;
                break;
                
            case 'k':
                if ( strcasecmp(optarg, "text") == 0 ) {
                    core_kind = kI8080MemCoreDumpKindText;
                }
                else if ( strcasecmp(optarg, "binary") == 0 ) {
                    core_kind = kI8080MemCoreDumpKindBinary;
                }
                else {
                    ERROR("Invalid core dump kind: %s", optarg);
                    exit(EINVAL);
                }
                break;
        
            case 'P':
            case kI8e8eOptAlternatePC:
                if ( ! I8080AddrParse(optarg, &PC, NULL) ) {
                    ERROR("Unable to parse program counter (PC) argument: %s", optarg);
                    exit(EINVAL);
                }
                break;
                
            case 'S':
            case kI8e8eOptAlternateSP:
                if ( ! I8080AddrParse(optarg, &SP, NULL) ) {
                    ERROR("Unable to parse stack pointer (SP) argument: %s", optarg);
                    exit(EINVAL);
                }
                break;
            
            case '2':
                INFO("Enabling 2 MHz clock accuracy in I8080System");
                system_opts |= kI8080SystemOpts2MHzClock;
                break;
            
            case 'A':
                INFO("Enabling accelerated instruction handling in I8080System");
                system_opts |= kI8080SystemOptsAccelInstr;
                break;
                break;
            
            case 't':
                if ( ! I8080DeviceTTYOptsParse(optarg, &tty_in_opts, &tty_out_opts) ) exit(EINVAL);
                break;
            
            case 'f': {
                I8e8eFileDeviceObj_t    *filedevobj = I8e8eFileDeviceObjParse(optarg);
                
                if ( ! filedevobj ) exit(EINVAL);
                if ( filedevobjs_tail ) {
                    filedevobjs_tail->link = filedevobj;
                    filedevobjs_tail = filedevobj;
                } else {
                    filedevobjs = filedevobjs_tail = filedevobj;
                }
                break;
            }
            
            case 'T': {
                if ( ! I8080AddrParse(optarg, &timer_addr, NULL) ) {
                    ERROR("Unable to parse realtime timer mapping address: %s", optarg);
                    exit(EINVAL);
                }
                have_timer = true;
                break;
            }
            
            case 'c': {
                if ( ! I8e8eCGAParse(optarg, &cga) ) exit(EINVAL);
                have_cga = true;
                break;
            }

#ifdef I8E8E_HAVE_PPU
            case 'g': {
                if ( ! I8e8ePPUParse(optarg, &ppu) ) exit(EINVAL);
                have_ppu = true;
                break;
            }
            
            case 'd': {
                if ( optarg ) {
                    char        *endptr;
                    long        v = strtol(optarg, &endptr, 0);
                    
                    if ( (endptr == optarg) || (v < 1) || (v > 255) ) {
                        ERROR("Invalid input device channel: %s", optarg);
                        exit(EINVAL);
                    }
                    dpad_dev_id = v;
                }
                have_dpad = true;
                break;
            }
            
            case 'p': {
                I8080CGAPaletteId_t     palette_id = kI8080CGAPaletteIdDefault;
                
                printf("\nAvailable CGA palettes (by id and name):\n\n");
                while ( palette_id < kI8080CGAPaletteIdMax ) {
                    printf(" %3d: %s\n", palette_id, I8080CGAPaletteIdStrs[palette_id]);
                    palette_id++;
                }
                fputc('\n', stdout);
                exit(0);
            }
#endif

#ifdef I8E8E_HAVE_APU
            case 'a': {
                if ( ! I8e8eAPUParse(optarg, &apu) ) exit(EINVAL);
                have_apu = true;
                break;
            }
#endif

        }
    }
    
#ifdef I8E8E_HAVE_PPU
    if ( have_ppu && have_cga ) {
        ERROR("CGA and PPU memory mapped devices cannot be used together");
        exit(EINVAL);
    }
#endif
    
    // Static stuff:
    I8080DeviceTTYContext_t         tty_context = {
                                        .stdin_context.options = tty_in_opts,
                                        .stdout_context.options = tty_out_opts };
    I8080Device_t                   devStderr;
                                        
    // Allocate the system so we have the device bus and
    // memory subsystem available:
    sys8080 = I8080SystemCreateWithTTYContext(
                    system_opts,
                    0,
                    &tty_context);
    
    if ( ! sys8080 ) {
        ERROR("Failed to allocate base system");
        exit(errno);
    }
    
    // File to attach for disassembly?
    if ( disasm_file && (system_opts & kI8080SystemOptsDisasmToFile) ) {
        sys8080->disasm_fptr = fopen(disasm_file, "w");
    }
    
    // Register stderr as output device #255:
    devStderr = *I8080DeviceStdError;
    I8080DevBusRegisterDevice(sys8080->devbus, 0xFF, &devStderr, NULL);
    
    // Register all file devices:
    if ( filedevobjs && ! I8e8eFileDeviceObjRegister(filedevobjs, sys8080) ) exit(EINVAL);

    // Walk the memory objects to attach ROMs and unmapped segments:
    if ( memobjs && ! I8e8eMemObjPreBootRegister(memobjs, rom_mapmode, sys8080) ) exit(EINVAL);
    
    // BSMs?
    if ( bsmobjs && ! I8e8eBSMObjRegister(bsmobjs, sys8080) ) exit(EINVAL);
    
    // Was CGA requested?
    if ( have_cga ) {
        I8080CGAMapperContextPtr    cga_context;
        I8080MemMapperRef_t         cga_mapper = { .callbacks = *I8080CGAMapperCallbacks };
        
        if ( cga.have_dims ) {
            cga_context = I8080CGAMapperContextCreateWithOriginAndSize(cga.mode, cga.x, cga.y, cga.w, cga.h);
        } else {
            cga_context = I8080CGAMapperContextCreateWithWindow(cga.mode, NULL);
        }
        cga_mapper.addr_range = I8080AddrRangeCreate(cga.base_addr, cga_context->n_pages_required << 8);
        cga_mapper.context = cga_context;
        if ( cga_context->n_pages_required > I8e8eCGASoftPageLimit ) {
            WARNING("CGA adapter requires $%02hX pages of memory", cga_context->n_pages_required);
        }
        if ( cga_context->n_pages_required > I8e8eCGAHardPageLimit ) {
            ERROR("CGA adapter would require more than $%02hX pages of memory", I8e8eCGAHardPageLimit);
            exit(ENOMEM);
        }
        cga_context->color_palette = I8080CGAPalettes[cga.palette_id];
        if ( ! I8080MemRegisterMapper(sys8080->sysmem, &cga_mapper) ) {
            exit(EINVAL);
        }
    }

#ifdef I8E8E_HAVE_PPU
    // Was PPU requested?
    if ( have_ppu ) {
        I8080PPUMapperContextPtr    ppu_context;
        I8080MemMapperRef_t         ppu_mapper = { .callbacks = *I8080PPUMapperCallbacks };
        
        ppu_context = I8080PPUMapperContextCreate(sys8080, ppu.palette_id);
        ppu_mapper.context = ppu_context;
        ppu_mapper.addr_range = I8080AddrRangeCreate(ppu.base_addr, I8080PPUMapperAddressRangeLength);
        if ( ! I8080MemRegisterMapper(sys8080->sysmem, &ppu_mapper) ) {
            exit(EINVAL);
        }
    }
    
    // D-pad?
    if ( have_dpad ) {
        if ( ! I8080DevBusRegisterDevice(sys8080->devbus, dpad_dev_id, I8080DeviceDPadIn, NULL) ) {
            exit(EINVAL);
        }
    }
#endif

#ifdef I8E8E_HAVE_APU
    // Was APU requested?
    if ( have_apu ) {
        I8080APUMapperContextPtr    apu_context;
        I8080MemMapperRef_t         apu_mapper = { .callbacks = *I8080APUMapperCallbacks };
        
        apu_context = I8080APUMapperContextCreate();
        apu_mapper.context = apu_context;
        apu_mapper.addr_range = I8080AddrRangeCreate(apu.base_addr, I8080APUMapperAddressRangeLength);
        if ( ! I8080MemRegisterMapper(sys8080->sysmem, &apu_mapper) ) {
            exit(EINVAL);
        }
    }
#endif
    
    // Realtime timer?
    if ( have_timer ) {
            I8080MemMapperRef_t     timer_mapper = { .addr_range = I8080AddrRangeCreate(timer_addr, 0x0100),
                                                    .callbacks = *I8080TimerMapperCallbacks };
        timer = I8080TimerContextCreate(sys8080);
        timer_mapper.context = timer;
        if ( ! I8080MemRegisterMapper(sys8080->sysmem, &timer_mapper) ) {
            exit(EINVAL);
        }
    }

    // Set power on:
    I8080SystemSetPowerState(sys8080, true);
    
    // Walk the memory objects to load data into system memory now:
    if ( memobjs && ! I8e8eMemObjPostBootRegister(memobjs, rom_mapmode, sys8080) ) exit(EINVAL);
    
    sys8080->rgstrs.SP = SP;
    INFO("Stack pointer (SP) set to $%04hX", SP);
    
    INFO("Running with program counter (PC) set to $%04hX", PC);
    I8080SystemRun(sys8080, PC);
    
    I8080TextBufferRef  summary = I8080TextBufferCreate();
    
    I8080SystemWriteToTextBuffer(summary, sys8080);
    
    if ( timer ) I8080TimerContextWriteToTextBuffer(summary, timer);
    
    if ( have_cga ) I8080CGAShutdown();
    I8080SystemSetPowerState(sys8080, false);
    
    if ( want_core_dump ) {
        FILE        *core_stream = fopen(want_core_dump, "wb");
        if ( core_stream ) {
            I8080MemCoreDump(sys8080->sysmem, core_kind, core_stream);
            fclose(core_stream);
        }
    }
    
    if ( sys8080->disasm_fptr && (system_opts & kI8080SystemOptsDisasmToFile) ) {
        fclose(sys8080->disasm_fptr);
    }
    
    I8080SystemDestroy(sys8080);
    
    if ( memobjs ) I8e8eMemObjDestroy(memobjs);
    if ( bsmobjs ) I8e8eBSMObjDestroy(bsmobjs);
    if ( filedevobjs ) I8e8eFileDeviceObjDestroy(filedevobjs);
    
    printf("\n%s\n", I8080TextBufferGetCStringPtr(summary));
    I8080TextBufferDestroy(summary);
    
    return 0;
}
