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
#include "I8e8eFileDeviceObj.h"
#include "I8080CGA.h"
#include "I8080PPU.h"
#include "I8080DPad.h"
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
        { "load",           required_argument,  0,  'L' },
        { "rom",            required_argument,  0,  'R' },
#ifdef HAS_MMAP
        { "rom-map-mode",   required_argument,  0,  'm' },
#endif
        { "unmapped",       required_argument,  0,  'U' },
        { "pc",             required_argument,  0,  'P' },
        { "PC",             required_argument,  0,  kI8e8eOptAlternatePC },
        { "sp",             required_argument,  0,  'S' },
        { "SP",             required_argument,  0,  kI8e8eOptAlternateSP },
        { "2mhz",           no_argument,        0,  '2' },
        { "file-device",    required_argument,  0,  'f' },
        { "tty",            required_argument,  0,  't' },
        { "timers",         required_argument,  0,  'T' },
        { "cga",            required_argument,  0,  'c' },
        { "ppu",            required_argument,  0,  'g' },
        { "dpad",           optional_argument,  0,  'd' },
        { "list-palettes",  no_argument,        0,  'p' },
        { NULL,             0,                  0,   0  }
    };

static const char *i8e8e_options_str = 
        "hvql:L:R:"
#ifdef HAS_MMAP
        "m:"
#endif
        "U:P:S:f:t:T:c:g:d:p";

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
            "    -l/--logfile <filepath>        redirect logging output to the file\n"
            "                                   at <filepath>; new lines are appended\n"
            "                                   to existing data in <filepath>\n"
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
            "    -P/--pc/--PC <addr>            on start, set the program counter (PC)\n"
            "                                   to this address (default: $0000)\n"
            "    -S/--sp/--SP <addr>            on start, set the stack pointer (SP)\n"
            "                                   to this address (default: $0000)\n"
            "    -2/--2mhz                      limit the instruction dispatch rate to\n"
            "                                   a clock frequency of 2 MHz; without this\n"
            "                                   flag instructions are dispatched as fast\n"
            "                                   as possible\n"
            "    -f/--file-device <file-dev>    connect a file to the device bus of the\n"
            "                                   emulator\n"
            "    -t/--tty <tty-options>         configure TTY options\n"
            "    -T/--timers <timers-options>   configure realtime timers\n"
            "    -c/--cga <cga-options>         configure a Curses Graphics Adapter...\n"
            "    -g/--ppu <ppu-options>         ...or configure a Picture Processing Unit\n"
            "    -d/--dpad                      attach a D-pad controller input device on\n"
            "                                   the first available input channel number\n"
            "    --dpad=<dev-id>                attach a D-pad on a specific input channel\n"
            "                                   number\n"
            "    -p/--list-palettes             list the available CGA color palettes\n"
            "                                   then exit\n"
            "\n"
            "    <addr> = ( $XXXX | 0xXXXX | 0N… | N… )\n\n"
            "        The emulator features a 64 KiB address space.  Addresses can be\n"
            "        expressed in dollar-sign notation or with a '0x' prefix for\n"
            "        hexedecimal values, or as an octal or decimal value.\n"
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
            "    <ppu-options> = @<addr>{:<palette-id>}\n"
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
    
    // Better start with an '@':
    if ( *in_str != '@' ) {
        ERROR("Invalid PPU specification: %s", in_str_orig);
        return false;
    }
    
    // Parse the address string:
     ++in_str;
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

typedef enum {
    kI8e8eROMMappingModeAlloc,
    kI8e8eROMMappingModeMMap
} I8e8eROMMappingMode_t;

bool
I8e8eROMMappingModeParse(
    const char              *in_str,
    I8e8eROMMappingMode_t   *out_mode
)
{
    if ( strcasecmp(in_str, "alloc") == 0 ) {
        *out_mode = kI8e8eROMMappingModeAlloc;
        return true;
    }
    if ( strcasecmp(in_str, "mmap") == 0 ) {
        *out_mode = kI8e8eROMMappingModeMMap;
        return true;
    }
    return false;
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
    I8080DeviceStdInputOpts_t   tty_in_opts = 0;
    I8080DeviceStdOutputOpts_t  tty_out_opts = 0;
    I8080Addr_t                 SP = 0x0000, PC=0x0000;
    I8e8eCGA_t                  cga;
    bool                        have_cga = false;
    I8e8ePPU_t                  ppu;
    bool                        have_ppu = false;
    I8080DevBusRegisterId       dpad_dev_id = kI8080DevBusRegisterIdNextAvail;
    bool                        have_dpad = false;
    I8e8eMemObj_t               *memobjs = NULL, *memobjs_tail = NULL;
    I8e8eROMMappingMode_t       rom_mapmode = kI8e8eROMMappingModeAlloc;
    I8e8eFileDeviceObj_t        *filedevobjs = NULL, *filedevobjs_tail = NULL;
    I8080Addr_t                 timer_addr;
    I8080TimerContextPtr        timer = NULL;
    bool                        have_timer = false;
    bool                        have_2mhz = false;
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
            
            case 'l':
                if ( ! I8080LoggingSetFile(optarg, true) ) {
                    ERROR("Unable to open file for logging: %s", optarg);
                    exit(errno);
                }
                break;
            
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
                have_2mhz = true;
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
        }
    }
    
    if ( have_ppu && have_cga ) {
        ERROR("CGA and PPU memory mapped devices cannot be used together");
        exit(EINVAL);
    }
    
    // Static stuff:
    I8080DeviceTTYContext_t         tty_context = {
                                        .stdin_context.options = tty_in_opts,
                                        .stdout_context.options = tty_out_opts };
    I8080Device_t                   devStderr;
                                        
    // Allocate the system so we have the device bus and
    // memory subsystem available:
    sys8080 = I8080SystemCreateWithTTYContext(
                    have_2mhz ? kI8080SystemOpts2MHzClock : 0,
                    0,
                    &tty_context);
    
    if ( ! sys8080 ) {
        ERROR("Failed to allocate base system");
        exit(errno);
    }
    
    // Register stderr as output device #255:
    devStderr = *I8080DeviceStdError;
    I8080DevBusRegisterDevice(sys8080->devbus, 0xFF, &devStderr, NULL);
    
    // Register all file devices:
    I8e8eFileDeviceObj_t    *this_filedev_obj = filedevobjs;
    
    while ( this_filedev_obj ) {
        switch ( this_filedev_obj->mode ) {
            case kI8080DeviceModeInput:
                this_filedev_obj->device = *I8080DeviceFileIn;
                break;
            case kI8080DeviceModeOutput:
                this_filedev_obj->device = *I8080DeviceFileOut;
                break;
            case kI8080DeviceModeInputOutput:
                this_filedev_obj->device = *I8080DeviceFileInOut;
                break;
        }
        this_filedev_obj->device_context.variant = kI8080DeviceFileVariantPath;
        this_filedev_obj->device_context.path.filepath = this_filedev_obj->filepath;
        this_filedev_obj->device_context.path.mode = this_filedev_obj->fopen_mode;
        if ( ! I8080DevBusRegisterDevice(sys8080->devbus, this_filedev_obj->dev_id,
                    &this_filedev_obj->device, &this_filedev_obj->device_context) )
        {
            exit(EINVAL);
        }
        this_filedev_obj = this_filedev_obj->link;
    }

    // Walk the memory objects to attach ROMs and unmapped segments:
    I8e8eMemObj_t   *this_mem_obj = memobjs;
    
    while ( this_mem_obj ) {
        I8080AddrRange_t        paged_addr = I8080AddrRangeToPages(this_mem_obj->mapper_data.addr_range);
        
        switch ( this_mem_obj->obj_type ) {
            case kI8e8eMemObjTypeData:
                break;
                
            case kI8e8eMemObjTypeROM: {
                I8080MemROMContextPtr   rom_context;
                switch ( rom_mapmode ) {
                    case kI8e8eROMMappingModeAlloc: {
                        rom_context = I8080MemROMContextWithByteRangeInFile(this_mem_obj->file_path, this_mem_obj->file_offset, this_mem_obj->file_length, paged_addr.length);
                        break;
                    }
                    
                    case kI8e8eROMMappingModeMMap: {
                        int     rom_fd = open(this_mem_obj->file_path, O_RDONLY);
                        
                        if ( rom_fd < 0 ) {
                            ERROR("Unable to open ROM file for mmap (errno=%d)", errno);
                            exit(errno);
                        }
                        rom_context = I8080MemROMContextWithMappedFile(rom_fd, this_mem_obj->file_offset,
                                            (this_mem_obj->file_length > 0xFFFF) ? 0xFFFF : this_mem_obj->file_length, paged_addr.length);
                        close(rom_fd);
                        break;
                    }
                    
                }
                if ( ! rom_context ) {
                    ERROR("Unable to allocate ROM context (errno=%d)", errno);
                    exit(errno);
                }
                this_mem_obj->mapper_data.callbacks = *I8080MemROMCallbacks;
                this_mem_obj->mapper_data.context = rom_context;
                rom_context->rom_name = this_mem_obj->file_path;
                if ( ! I8080MemRegisterMapper(sys8080->sysmem, &this_mem_obj->mapper_data) ) {
                    exit(ENOMEM);
                }
                INFO("Created new %s ROM image @ $%04hX - $%04hX", 
                    (rom_mapmode == kI8e8eROMMappingModeAlloc) ? "allocated buffer" : "memory-mapped",
                    this_mem_obj->mapper_data.addr_range.base, I8080AddrRangeEndAddr(this_mem_obj->mapper_data.addr_range));
                break;
            }
            case kI8e8eMemObjTypeUnmapped: {
                this_mem_obj->mapper_data.callbacks = *I8080MemUnmappedSegmentCallbacks;
                this_mem_obj->mapper_data.context = (const void*)(uintptr_t)this_mem_obj->unmapped_byte;
                if ( ! I8080MemRegisterMapper(sys8080->sysmem, &this_mem_obj->mapper_data) ) {
                    exit(ENOMEM);
                }
                INFO("Created new unmapped segment @ $%04hX - $%04hX (byte = 0x%02hhX)", 
                        this_mem_obj->mapper_data.addr_range.base, I8080AddrRangeEndAddr(this_mem_obj->mapper_data.addr_range), this_mem_obj->unmapped_byte);
                break;
            }
        }
        this_mem_obj = this_mem_obj->link;
    }
    
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
    
    // Walk the memory objects to load data now:
    this_mem_obj = memobjs;
    while ( this_mem_obj ) {
        switch ( this_mem_obj->obj_type ) {
            case kI8e8eMemObjTypeData: {
                FILE        *fptr = fopen(this_mem_obj->file_path, "rb");
                bool        full_length = (this_mem_obj->file_length == 0);
                I8080Addr_t addr = this_mem_obj->mapper_data.addr_range.base;
                size_t      n_bytes = 0;
                
                if ( this_mem_obj->mapper_data.addr_range.length > 0 ) {
                    // An address range was provided, does it limit the read?
                    if ( full_length || (this_mem_obj->mapper_data.addr_range.length < this_mem_obj->file_length) ) {
                        this_mem_obj->file_length = this_mem_obj->mapper_data.addr_range.length;
                        full_length = false;
                    } 
                }
                
                if ( ! fptr ) {
                    ERROR("Unable to open file `%s` to load data to system memory (errno=%d)",
                            this_mem_obj->file_path, errno);
                    exit(errno);
                }
                if ( this_mem_obj->file_offset ) {
                    if ( fseeko(fptr, this_mem_obj->file_offset, SEEK_SET) ) {
                        ERROR("Unable to move to offset %lld in `%s` to load data to system memory (errno=%d)",
                            this_mem_obj->file_offset, this_mem_obj->file_path, errno);
                        exit(errno);
                    }
                }
                while ( full_length || (this_mem_obj->file_length > 0) ) {
                    uint8_t     byte;
                    
                    if ( fread(&byte, 1, 1, fptr) != 1 ) break;
                    I8080MemWrite(sys8080->sysmem, addr++, byte);
                    this_mem_obj->file_length--, n_bytes++;
                }
                if ( ! full_length && this_mem_obj->file_length > 0 ) {
                    ERROR("Unable to read all bytes from `%s` (%lld unread)", this_mem_obj->file_path, this_mem_obj->file_length);
                    exit(EINVAL);
                }
                fclose(fptr);
                INFO("Added %lld bytes to system memory starting at $%04hX\n", n_bytes, this_mem_obj->mapper_data.addr_range.base);
                break;
            }
            case kI8e8eMemObjTypeROM:
            case kI8e8eMemObjTypeUnmapped: 
                break;
        }
        this_mem_obj = this_mem_obj->link;
    }
    
    sys8080->rgstrs.SP = SP;
    INFO("Stack pointer (SP) set to $%04hX", SP);
    
    INFO("Running with program counter (PC) set to $%04hX", PC);
    I8080SystemRun(sys8080, PC);
    
    I8080TextBufferRef  summary = I8080TextBufferCreate();
    
    I8080RegistersWriteToTextBuffer(summary, &sys8080->rgstrs);
    I8080DevBusWriteToTextBuffer(summary, sys8080->devbus);
    I8080MemWriteToTextBuffer(summary, sys8080->sysmem);
    if ( timer ) I8080TimerContextWriteToTextBuffer(summary, timer);
    
    if ( have_cga ) I8080CGAShutdown();
    I8080SystemSetPowerState(sys8080, false);
    I8080SystemDestroy(sys8080);
    
    printf("\n%s\n", I8080TextBufferGetCStringPtr(summary));
    I8080TextBufferDestroy(summary);
    
    return 0;
}
