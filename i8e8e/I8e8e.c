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
#include <fcntl.h>
#include <signal.h>
#include <getopt.h>

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
        { "file-device",    required_argument,  0,  'f' },
        { "tty",            required_argument,  0,  't' },
        { NULL,             0,                  0,   0  }
    };

static const char *i8e8e_options_str = 
        "hvql:L:R:"
#ifdef HAS_MMAP
        "m:"
#endif
        "U:P:S:f:t:";

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
            "    -f/--file-device <file-dev>    connect a file to the device bus of the\n"
            "                                   emulator\n"
            "    -t/--tty <tty-options>         configure TTY options\n"
            "\n"
            "    <addr> = ( $XXXX | 0xXXXX | 0N… | N… )\n\n"
            "        The emulator features a 64 KiB address space.  Addresses can be\n"
            "        expressed in dollar-sign notation or with a '0x' prefix for\n"
            "        hexedecimal values, or as an octal or decimal value.\n"
            "\n"
            "    <offset>, <len>, <byte> = { $X… | 0xX… | 0N… | N… }\n\n"
            "        An <offset>, <len>, or <byte> are a hexadecimal, octal, or decimal value\n"
            "\n"
            "    <bytes-in> = <filepath>{:<offset>}{#<len>}@<addr>\n\n"
            "        Bytes are read from <filepath> starting at <offset> (or the\n"
            "        beginnining of the file if <offset> is not provided) and inserted\n"
            "        into the system memory of the emulator starting at <addr>.  Up to\n"
            "        <len> bytes are inserted (or the full length of the file after\n"
            "        <offset> if <len> is omitted) with the operation wrapping back around\n"
            "        the address space as necessary.\n"
            "\n"
#ifdef HAS_MMAP
            "    <rom-mode> = ( alloc | mmap )\n\n"
            "        When ROM images are created, the file contents can either be loaded into\n"
            "        an allocated buffer in the emulator's memory space or the kernel can map\n"
            "        the byte range indirectly via mmap()\n"
            "\n"
#endif
            "    <unmapped-seg> = <addr>-<addr>:<byte>\n\n"
            "        An unmapped segment is created for which <byte> will be returned on\n"
            "        read and writes will be ignored.  The address range starts at the first\n"
            "        <addr> and goes through the second <addr>, inclusive\n"
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
    I8080SystemInterrupt(sys8080);
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
    I8e8eMemObj_t              *memobjs = NULL, *memobjs_tail = NULL;
    I8e8eROMMappingMode_t       rom_mapmode = kI8e8eROMMappingModeAlloc;
    I8e8eFileDeviceObj_t        *filedevobjs = NULL, *filedevobjs_tail = NULL;
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
        }
    }
    
    // Static stuff:
    I8080DeviceTTYContext_t         tty_context = {
                                        .stdin_context.options = tty_in_opts,
                                        .stdout_context.options = tty_out_opts };
    I8080Device_t                   devStderr;
                                        
    // Allocate the system so we have the device bus and
    // memory subsystem available:
    sys8080 = I8080SystemCreateWithTTYContext(0, 0, &tty_context);
    
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
    const I8080MemCallbacks *last_callbacks = NULL;
    const void              *last_context = NULL;
    I8e8eMemObj_t           *this_mem_obj = memobjs;
    
    while ( this_mem_obj ) {
        switch ( this_mem_obj->obj_type ) {
            case kI8e8eMemObjTypeData:
                break;
                
            case kI8e8eMemObjTypeROM: {
                this_mem_obj->callbacks = *I8080MemROMCallbacks;
                switch ( rom_mapmode ) {
                    case kI8e8eROMMappingModeAlloc:
                        this_mem_obj->context.rom = I8080MemROMWithByteRangeInFile(this_mem_obj->addr, this_mem_obj->filepath,
                                                    this_mem_obj->offset, this_mem_obj->len);
                        break;
#ifdef HAS_MMAP
                    case kI8e8eROMMappingModeMMap: {
                        int     rom_fd = open(this_mem_obj->filepath, O_RDONLY);
                        
                        if ( rom_fd < 0 ) {
                            ERROR("Unable to open ROM file for mmap (errno=%d)", errno);
                            exit(errno);
                        }
                        this_mem_obj->context.rom = I8080MemROMWithMappedFile(this_mem_obj->addr, rom_fd, this_mem_obj->offset,
                                                    (this_mem_obj->len > 0xFFFF) ? 0xFFFF : this_mem_obj->len);
                        close(rom_fd);
                        break;
                    }
#endif 
                }
                if ( ! this_mem_obj->context.rom ) {
                    ERROR("Unable to allocate ROM context (errno=%d)", errno);
                    exit(errno);
                }
                this_mem_obj->context.rom->rom_name = this_mem_obj->filepath;
                this_mem_obj->context.rom->next_callbacks = last_callbacks;
                this_mem_obj->context.rom->next_context = last_context;
                last_callbacks = &this_mem_obj->callbacks;
                last_context = this_mem_obj->context.rom;
                INFO("Created new %s ROM image @ $%04hX - $%04hX", 
                    (rom_mapmode == kI8e8eROMMappingModeAlloc) ? "allocated buffer" : "memory-mapped",
                    this_mem_obj->context.rom->rom_addr, this_mem_obj->context.rom->rom_addr + this_mem_obj->context.rom->rom_size - 1);
                break;
            }
            case kI8e8eMemObjTypeUnmapped: {
                this_mem_obj->callbacks = *I8080MemUnmappedCallbacks;
                this_mem_obj->context.unmapped = (I8080MemUnmappedContext_t*)calloc(1, sizeof(I8080MemUnmappedContext_t));
                if ( ! this_mem_obj->context.unmapped ) {
                    ERROR("Unable to allocate unmapped segment context (errno=%d)", errno);
                    exit(errno);
                }
                this_mem_obj->context.unmapped->base_address = this_mem_obj->addr;
                this_mem_obj->context.unmapped->unmapped_size = this_mem_obj->len;
                this_mem_obj->context.unmapped->unmapped_byte = this_mem_obj->byte;
                this_mem_obj->context.unmapped->next_callbacks = last_callbacks;
                this_mem_obj->context.unmapped->next_context = last_context;
                last_callbacks = &this_mem_obj->callbacks;
                last_context = this_mem_obj->context.unmapped;
                INFO("Created new unmapped segment @ $%04hX - $%04hX (byte = 0x%02hhX)", 
                        this_mem_obj->context.unmapped->base_address,
                        this_mem_obj->context.unmapped->base_address + this_mem_obj->context.unmapped->unmapped_size,
                        this_mem_obj->context.unmapped->unmapped_byte);
                break;
            }
        }
        this_mem_obj = this_mem_obj->link;
    }
    
    // If we setup any callbacks and contexts, attach them now:
    if ( last_callbacks ) {
        I8080MemSetCallbacks(sys8080->sysmem, last_callbacks, last_context);
    }

    // Set power on:
    I8080SystemSetPowerState(sys8080, true);
    
    // Walk the memory objects to load data now:
    this_mem_obj = memobjs;
    while ( this_mem_obj ) {
        switch ( this_mem_obj->obj_type ) {
            case kI8e8eMemObjTypeData: {
                FILE        *fptr = fopen(this_mem_obj->filepath, "rb");
                bool        full_length = (this_mem_obj->len == 0);
                I8080Addr_t addr = this_mem_obj->addr;
                size_t      n_bytes = 0;
                
                if ( ! fptr ) {
                    ERROR("Unable to open file `%s` to load data to system memory (errno=%d)",
                            this_mem_obj->filepath, errno);
                    exit(errno);
                }
                if ( this_mem_obj->offset ) {
                    if ( fseeko(fptr, this_mem_obj->offset, SEEK_SET) ) {
                        ERROR("Unable to move to offset %lld in `%s` to load data to system memory (errno=%d)",
                            this_mem_obj->offset, this_mem_obj->filepath, errno);
                        exit(errno);
                    }
                }
                while ( full_length || (this_mem_obj->len > 0) ) {
                    uint8_t     byte;
                    
                    if ( fread(&byte, 1, 1, fptr) != 1 ) break;
                    I8080MemWrite(sys8080->sysmem, addr++, byte);
                    this_mem_obj->len--, n_bytes++;
                }
                if ( ! full_length && this_mem_obj->len > 0 ) {
                    ERROR("Unable to read all bytes from `%s` (%lld unread)", this_mem_obj->filepath, this_mem_obj->len);
                    exit(EINVAL);
                }
                fclose(fptr);
                INFO("Added %lld bytes to system memory starting at $%04hX\n", n_bytes, this_mem_obj->addr);
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
    
    printf("\n");
    I8080RegistersPrint(stdout, &sys8080->rgstrs);
    I8080DevBusPrint(stdout, sys8080->devbus);
    
    I8080SystemSetPowerState(sys8080, false);
    I8080SystemDestroy(sys8080);
    
    return 0;
}
