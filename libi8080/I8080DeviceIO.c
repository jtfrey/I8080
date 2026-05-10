/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 device i/o API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */

#include "I8080DeviceIO.h"
#include "I8080Logging.h"

typedef struct {
    I8080Device_t       *device_ptr;
    const void          *context;
} I8080DevNode_t;

typedef struct I8080DevBus {
    uint8_t             n_input;
    uint8_t             n_output;
    I8080DevNode_t      input_devs[256];
    I8080DevNode_t      output_devs[256];
} I8080DevBus_t;

//

I8080DevBusRef
I8080DevBusCreate(void)
{
    I8080DevBus_t       *devbus = (I8080DevBus_t*)calloc(1, sizeof(I8080DevBus_t));
    
    if ( devbus ) {
        INFO("Created device bus %p", devbus);
    }
    return devbus;
}

//

void
I8080DevBusDestroy(
    I8080DevBusRef  devbus
)
{
    // Unregister everything after doing a shutdown:
    int             i = 0;
    
    I8080DevBusShutdown(devbus);
    while ( (i < devbus->n_input) && (i < devbus->n_output) ) {
        if ( devbus->input_devs[i].device_ptr ) {
            devbus->input_devs[i].device_ptr->input.device_id = 0;
            DEBUG("Unregistered input device 0x%02hhX on device bus %p", i, devbus);
        }
        if ( devbus->output_devs[i].device_ptr ) {
            devbus->output_devs[i].device_ptr->output.device_id = 0;
            DEBUG("Unregistered output device 0x%02hhX on device bus %p", i, devbus);
        }
        i++;
    }
    while ( i < devbus->n_input ) {
        if ( devbus->input_devs[i].device_ptr ) {
            devbus->input_devs[i].device_ptr->input.device_id = 0;
            DEBUG("Unregistered input device 0x%02hhX on device bus %p", i, devbus);
        }
        i++;
    }
    while ( i < devbus->n_output ) {
        if ( devbus->output_devs[i].device_ptr ) {
            devbus->output_devs[i].device_ptr->output.device_id = 0;
            DEBUG("Unregistered output device 0x%02hhX on device bus %p", i, devbus);
        }
        i++;
    }
    free((void*)devbus);
    DEBUG("Destroyed device bus %p", devbus);
}

//

void
I8080DevBusReset(
    I8080DevBusRef  devbus
)
{
    int             i = 0;
    
    while ( i < 256 ) {
        bool        same_device = devbus->input_devs[i].device_ptr == devbus->output_devs[i].device_ptr;
        
        if ( devbus->input_devs[i].device_ptr ) {
            devbus->input_devs[i].device_ptr->input.bytes_in = 0;
            if ( ! same_device && devbus->input_devs[i].device_ptr->reset)
                devbus->input_devs[i].device_ptr->reset(devbus->input_devs[i].device_ptr, devbus->input_devs[i].context);
        }
        if ( devbus->output_devs[i].device_ptr ) {
            devbus->output_devs[i].device_ptr->output.bytes_out = 0;
            if ( ! same_device && devbus->output_devs[i].device_ptr->reset)
                devbus->output_devs[i].device_ptr->reset(devbus->output_devs[i].device_ptr, devbus->output_devs[i].context);
        }
        if ( same_device && devbus->input_devs[i].device_ptr && devbus->input_devs[i].device_ptr->reset )
            devbus->input_devs[i].device_ptr->reset(devbus->input_devs[i].device_ptr, devbus->input_devs[i].context);
        i++;
    }
}

//

void
I8080DevBusShutdown(
    I8080DevBusRef  devbus
)
{
    int             i = 0;
    
    while ( i < 256 ) {
        bool        same_device = devbus->input_devs[i].device_ptr == devbus->output_devs[i].device_ptr;
        
        if ( devbus->input_devs[i].device_ptr ) {
            devbus->input_devs[i].device_ptr->input.bytes_in = 0;
            if ( ! same_device && devbus->input_devs[i].device_ptr->shutdown)
                devbus->input_devs[i].device_ptr->shutdown(devbus->input_devs[i].device_ptr, devbus->input_devs[i].context);
        }
        if ( devbus->output_devs[i].device_ptr ) {
            devbus->output_devs[i].device_ptr->output.bytes_out = 0;
            if ( ! same_device && devbus->output_devs[i].device_ptr->shutdown)
                devbus->output_devs[i].device_ptr->shutdown(devbus->output_devs[i].device_ptr, devbus->output_devs[i].context);
        }
        if ( same_device && devbus->input_devs[i].device_ptr && devbus->input_devs[i].device_ptr->shutdown )
            devbus->input_devs[i].device_ptr->shutdown(devbus->input_devs[i].device_ptr, devbus->input_devs[i].context);
        i++;
    }
}

//

bool
I8080DevBusRegisterDevice(
    I8080DevBusRef          devbus,
    I8080DevBusRegisterId   req_id, 
    I8080Device_t           *dev,
    const void              *context
)
{
    bool            ok = true, did_add_input_dev = false;
    
    if ( dev->device_mode & kI8080DeviceModeInput ) {
        if ( devbus->n_input < 0xFF ) {
            I8080DevId  input_id = req_id & 0xFF;
            
            if ( req_id == kI8080DevBusRegisterIdNextAvail ) {
                input_id = 0;
                while  ( devbus->input_devs[input_id].device_ptr ) input_id++;
            } else if ( devbus->input_devs[input_id].device_ptr ) {
                ERROR("A device is already registered with input id 0x%02hhX", input_id);
                ok = false;
            }
            if ( ok ) {
                devbus->n_input++;
                dev->input.device_id = input_id;
                dev->input.bytes_in = 0LL;
                devbus->input_devs[input_id].device_ptr = dev;
                devbus->input_devs[input_id].context = context;
                did_add_input_dev = true;
                INFO("Input device \"%s\" registered as 0x%02hhX with device bus %p",
                    dev->device_name ? dev->device_name : "<UNKNOWN>",
                    input_id,
                    devbus);
            }
        } else {
            ERROR("Attempted to register more than 256 input devices");
            ok = false;
        }
    }
    if ( ok && (dev->device_mode & kI8080DeviceModeOutput) ) { 
        if ( devbus->n_output < 0xFF ) {
            I8080DevId  output_id = req_id & 0xFF;
            
            if ( req_id == kI8080DevBusRegisterIdNextAvail ) {
                output_id = 0;
                while  ( devbus->output_devs[output_id].device_ptr ) output_id++;
            } else if ( devbus->output_devs[output_id].device_ptr ) {
                ERROR("A device is already registered with output id 0x%02hhX", output_id);
                ok = false;
            }
            if ( ok ) {
                devbus->n_output++;
                dev->output.device_id = output_id;
                dev->output.bytes_out = 0LL;
                devbus->output_devs[output_id].device_ptr = dev;
                devbus->output_devs[output_id].context = context;
                INFO("Output device \"%s\" registered as 0x%02hhX with device bus %p",
                    dev->device_name ? dev->device_name : "<UNKNOWN>",
                    output_id,
                    devbus);
            }
        } else {
            ERROR("Attempted to register more than 256 output devices");
            ok = false;
            // Drop the input registration if it was made:
            if ( did_add_input_dev ) {
                devbus->input_devs[--devbus->n_input].device_ptr = NULL;
                devbus->input_devs[--devbus->n_input].context = NULL;
                dev->input.device_id = 0;
            }
        }
    }
    return ok;
}

//

void
I8080DevBusUnregisterDevice(
    I8080DevBusRef  devbus,
    I8080Device_t   *dev
)
{
    const void      *input_context = NULL;
    bool            did_shutdown_input = false;
    
    if ( dev->device_mode & kI8080DeviceModeInput ) {
        if ( dev->shutdown ) {
            input_context = devbus->input_devs[dev->input.device_id].context;
            dev->shutdown(dev, input_context);
            did_shutdown_input = true;
        }
        INFO("Input device \"%s\" shutdown and registered as 0x%02hhX on device bus %p",
            dev->device_name ? dev->device_name : "<UNKNOWN>",
            dev->input.device_id,
            devbus);
        devbus->input_devs[dev->input.device_id].device_ptr = NULL;
        devbus->input_devs[dev->input.device_id].context = NULL;
        if ( dev->input.device_id == devbus->n_input ) devbus->n_input--;
        dev->input.device_id = 0;
    }
    if ( dev->device_mode & kI8080DeviceModeOutput ) {
        if ( dev->shutdown && (! did_shutdown_input || (devbus->output_devs[dev->output.device_id].context != input_context)) ) {
            dev->shutdown(dev, devbus->output_devs[dev->output.device_id].context);
        }
        INFO("Output device \"%s\" shutdown and registered as 0x%02hhX with device bus %p",
            dev->device_name ? dev->device_name : "<UNKNOWN>",
            dev->output.device_id,
            devbus);
        devbus->output_devs[dev->output.device_id].device_ptr = NULL;
        devbus->output_devs[dev->output.device_id].context = NULL;
        if ( dev->output.device_id == devbus->n_output ) devbus->n_output--;
        dev->output.device_id = 0;
    }
}

//

uint8_t
I8080DevBusReadDevice(
    I8080DevBusRef  devbus,
    I8080DevId      dev_id
)
{
    uint8_t         byte = 0b00000000;
    
    if ( devbus->input_devs[dev_id].device_ptr ) {
        I8080DevNode_t  *dev = &devbus->input_devs[dev_id];
        
        dev->device_ptr->input.bytes_in++;
        byte = dev->device_ptr->input.read(dev->device_ptr, dev->context);
    } else {
        WARNING("Read from unregistered device id 0x%02hhX", dev_id);
    }
    return byte;
}

//

void
I8080DevBusWriteDevice(
    I8080DevBusRef  devbus,
    I8080DevId      dev_id,
    uint8_t         byte
)
{
   if ( devbus->output_devs[dev_id].device_ptr ) {
        I8080DevNode_t  *dev = &devbus->output_devs[dev_id];
        
        dev->device_ptr->output.bytes_out++;
        dev->device_ptr->output.write(dev->device_ptr, byte, dev->context);
    } else {
        WARNING("Write to unregistered device id 0x%02hhX", dev_id);
    }
}

//

void
I8080DevBusPrint(
    FILE            *stream,
    I8080DevBusRef  devbus
)
{
    int             i = 0;
    
    while ( i < 256 ) {
        if ( devbus->input_devs[i].device_ptr && devbus->output_devs[i].device_ptr && 
             (devbus->input_devs[i].device_ptr == devbus->output_devs[i].device_ptr) )
        {
            fprintf(stream, "I8080Device[%02X] [←%08lX|→%08lX] BYTES  \"%s\"\n",
                        i, devbus->input_devs[i].device_ptr->input.bytes_in, devbus->output_devs[i].device_ptr->output.bytes_out,
                        devbus->input_devs[i].device_ptr->device_name);
        } else {
            if ( devbus->input_devs[i].device_ptr ) {
                fprintf(stream, "I8080Device[%02X] [←%08lX|         ] BYTES  \"%s\"\n",
                        i, devbus->input_devs[i].device_ptr->input.bytes_in,
                        devbus->input_devs[i].device_ptr->device_name);
            }
            if ( devbus->output_devs[i].device_ptr ) {
                fprintf(stream, "I8080Device[%02X] [         |→%08lX] BYTES  \"%s\"\n",
                        i, devbus->output_devs[i].device_ptr->output.bytes_out,
                        devbus->output_devs[i].device_ptr->device_name);
            }
        }
        i++;
    }
}
    

//

#define kI8080DeviceStdIOOptsHasBeenConfigured 0x80000000

static
void
I8080DeviceStdInputReset(
    I8080DevicePtr  dev,
    const void      *context
)
{
    I8080DeviceStdInputContext_t    *stdin_context = (I8080DeviceStdInputContext_t*)context;
    
    if ( stdin_context && ! (stdin_context->options & kI8080DeviceStdIOOptsHasBeenConfigured) ) {
        struct termios ttystate;
        
        //get the terminal state
        tcgetattr(STDIN_FILENO, &ttystate);
        stdin_context->saved_state = ttystate;
        
        if ( stdin_context->options & kI8080DeviceStdInputOptsSetNonCanonicalMode ) {
            //turn off canonical mode
            ttystate.c_lflag &= ~(ICANON);
            //minimum of number input read.
            ttystate.c_cc[VMIN] = 1;
        }
        if ( stdin_context->options & kI8080DeviceStdInputOptsDisableEcho ) {
            //turn off echo modes
            ttystate.c_lflag &= ~(ECHO|ECHONL);
        }
        
        //set the terminal attributes.
        tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
        
        stdin_context->options |= kI8080DeviceStdIOOptsHasBeenConfigured;
    }
}

static
void
I8080DeviceStdInputShutdown(
    I8080DevicePtr  dev,
    const void      *context
)
{
    I8080DeviceStdInputContext_t    *stdin_context = (I8080DeviceStdInputContext_t*)context;
    
    if ( stdin_context && (stdin_context->options & kI8080DeviceStdIOOptsHasBeenConfigured) ) {
        //set the terminal attributes.
        tcsetattr(STDIN_FILENO, TCSANOW, &stdin_context->saved_state);
        stdin_context->options &= ~kI8080DeviceStdIOOptsHasBeenConfigured;
    }
}

static
uint8_t
I8080DeviceStdInputRead(
    I8080DevicePtr  dev,
    const void      *context
)
{
    return fgetc(stdin);
}

static
void
I8080DeviceStdOutputReset(
    I8080DevicePtr  dev,
    const void      *context
)
{
    I8080DeviceStdOutputContext_t   *stdout_context = (I8080DeviceStdOutputContext_t*)context;
    
    if ( stdout_context ) {
#ifdef HAS_TERMIOS_OLCUC
        if ( ! (stdout_context->options & kI8080DeviceStdIOOptsHasBeenConfigured) ) {
            struct termios ttystate;
            
            //get the terminal state
            tcgetattr(STDOUT_FILENO, &ttystate);
            stdout_context->saved_state = ttystate;
            
            if ( stdout_context->options & kI8080DeviceStdOutputOptsForceUppercase ) {
                //turn on forced uppercase
                ttystate.c_oflag |= OLCUC;
            }
            
            //set the terminal attributes.
            tcsetattr(STDOUT_FILENO, TCSANOW, &ttystate);
            
            stdout_context->options |= kI8080DeviceStdIOOptsHasBeenConfigured;
        }
#endif
        if ( stdout_context->options & kI8080DeviceStdOutputOptsClearScreenOnReset ) {
            printf("\033[H\033[2J");fflush(stdout);
        }
    }
}

static
void
I8080DeviceStdOutputShutdown(
    I8080DevicePtr  dev,
    const void      *context
)
{
#ifdef HAS_TERMIOS_OLCUC
    I8080DeviceStdOutputContext_t   *stdout_context = (I8080DeviceStdOutputContext_t*)context;
    
    if ( stdout_context && (stdout_context->options & kI8080DeviceStdIOOptsHasBeenConfigured) ) {
        //set the terminal attributes.
        tcsetattr(STDOUT_FILENO, TCSANOW, &stdout_context->saved_state);
        stdout_context->options &= ~kI8080DeviceStdIOOptsHasBeenConfigured;
    }
#endif
}

static
void
I8080DeviceStdOutputWrite(
    I8080DevicePtr  dev,
    uint8_t         byte,
    const void      *context
)
{
    fputc(byte, stdout);
}

static
void
I8080DeviceStdErrorWrite(
    I8080DevicePtr  dev,
    uint8_t         byte,
    const void      *context
)
{
    fputc(byte, stderr);
}

static
void
I8080DeviceTTYReset(
    I8080DevicePtr  dev,
    const void      *context
)
{
    I8080DeviceTTYContext_t *tty_context = (I8080DeviceTTYContext_t*)context;
    
    if ( tty_context ) {
        I8080DeviceStdInputReset(dev, &tty_context->stdin_context);
        I8080DeviceStdOutputReset(dev, &tty_context->stdout_context);
    }
}

static
void
I8080DeviceTTYShutdown(
    I8080DevicePtr  dev,
    const void      *context
)
{
    I8080DeviceTTYContext_t *tty_context = (I8080DeviceTTYContext_t*)context;
    
    if ( tty_context ) {
        I8080DeviceStdInputShutdown(dev, &tty_context->stdin_context);
        I8080DeviceStdOutputShutdown(dev, &tty_context->stdout_context);
    }
}

//

I8080Device_t __I8080DeviceStdInput = {
            .device_name = "stdin",
            .device_mode = kI8080DeviceModeInput,
            .input = {
                .read = I8080DeviceStdInputRead },
            .reset = I8080DeviceStdInputReset,
            .shutdown = I8080DeviceStdInputShutdown
        };
const I8080DevicePtr I8080DeviceStdInput = &__I8080DeviceStdInput;

I8080Device_t __I8080DeviceStdOutput = {
            .device_name = "stdout",
            .device_mode = kI8080DeviceModeOutput,
            .output = {
                .write = I8080DeviceStdOutputWrite },
            .reset = I8080DeviceStdOutputReset,
            .shutdown = I8080DeviceStdOutputShutdown
        };
const I8080DevicePtr I8080DeviceStdOutput = &__I8080DeviceStdOutput;

I8080Device_t __I8080DeviceStdError = {
            .device_name = "stderr",
            .device_mode = kI8080DeviceModeOutput,
            .output = {
                .write = I8080DeviceStdErrorWrite }
        };
const I8080DevicePtr I8080DeviceStdError = &__I8080DeviceStdError;

I8080Device_t __I8080DeviceTTY = {
            .device_name = "tty",
            .device_mode = kI8080DeviceModeInputOutput,
            .input = {
                .read = I8080DeviceStdInputRead },
            .output = {
                .write = I8080DeviceStdOutputWrite },
            .reset = I8080DeviceTTYReset,
            .shutdown = I8080DeviceTTYShutdown
        };
const I8080DevicePtr I8080DeviceTTY = &__I8080DeviceTTY;

//

static
void
I8080DeviceFileInOutReset(
    I8080DevicePtr  dev,
    const void      *context
)
{
    I8080DeviceFileContext_t    *file_context = (I8080DeviceFileContext_t*)context;
    
    switch ( file_context->variant ) {
        case kI8080DeviceFileVariantPath: {
            FILE    *fptr = fopen(file_context->path.filepath, file_context->path.mode);
            if ( ! fptr ) {
                ERROR("Unable to open file `%s` in mode `%s`", file_context->path.filepath, file_context->path.mode);
            } else {
                file_context->path.fptr = fptr;
            }
            break;
        }
        case kI8080DeviceFileVariantFILEPtr:
            break;
    }
}

//

static
void
I8080DeviceFileInOutShutdown(
    I8080DevicePtr  dev,
    const void      *context
)
{
    I8080DeviceFileContext_t    *file_context = (I8080DeviceFileContext_t*)context;
    
    switch ( file_context->variant ) {
        case kI8080DeviceFileVariantPath: {
            if ( file_context->path.fptr ) {
                fflush(file_context->path.fptr);
                fclose(file_context->path.fptr);
                file_context->path.fptr = NULL;
            }
            break;
        }
        case kI8080DeviceFileVariantFILEPtr: {
            if ( file_context->fileptr.should_close_on_shutdown && file_context->fileptr.fptr ) {
                fflush(file_context->fileptr.fptr);
                fclose(file_context->fileptr.fptr);
                file_context->fileptr.fptr = NULL;
            }
            break;
        }
    }
}

//

static
uint8_t
I8080DeviceFileInOutRead(
    I8080DevicePtr  dev,
    const void      *context
)
{
    I8080DeviceFileContext_t    *file_context = (I8080DeviceFileContext_t*)context;
    
    switch ( file_context->variant ) {
        case kI8080DeviceFileVariantPath: {
            if ( file_context->path.fptr ) {
                return fgetc(file_context->path.fptr);
            }
            break;
        }
        case kI8080DeviceFileVariantFILEPtr: {
            if ( file_context->fileptr.fptr ) {
                return fgetc(file_context->fileptr.fptr);
            }
            break;
        }
    }
    return 0xFF;
}

//

static
void
I8080DeviceFileInOutWrite(
    I8080DevicePtr  dev,
    uint8_t         byte,
    const void      *context
)
{
    I8080DeviceFileContext_t    *file_context = (I8080DeviceFileContext_t*)context;
    
    switch ( file_context->variant ) {
        case kI8080DeviceFileVariantPath: {
            if ( file_context->path.fptr ) {
                fputc(byte, file_context->path.fptr);
            }
            break;
        }
        case kI8080DeviceFileVariantFILEPtr: {
            if ( file_context->fileptr.fptr ) {
                fputc(byte, file_context->fileptr.fptr);
            }
            break;
        }
    }
}

//

I8080Device_t __I8080DeviceFileIn = {
            .device_name = "file-in",
            .device_mode = kI8080DeviceModeInput,
            .input = {
                .read = I8080DeviceFileInOutRead },
            .output = {
                .write = NULL },
            .reset = I8080DeviceFileInOutReset,
            .shutdown = I8080DeviceFileInOutShutdown
        };
const I8080DevicePtr I8080DeviceFileIn = &__I8080DeviceFileIn;

I8080Device_t __I8080DeviceFileOut = {
            .device_name = "file-out",
            .device_mode = kI8080DeviceModeOutput,
            .input = {
                .read = NULL },
            .output = {
                .write = I8080DeviceFileInOutWrite },
            .reset = I8080DeviceFileInOutReset,
            .shutdown = I8080DeviceFileInOutShutdown
        };
const I8080DevicePtr I8080DeviceFileOut = &__I8080DeviceFileOut;

I8080Device_t __I8080DeviceFileInOut = {
            .device_name = "file-in+out",
            .device_mode = kI8080DeviceModeInputOutput,
            .input = {
                .read = I8080DeviceFileInOutRead },
            .output = {
                .write = I8080DeviceFileInOutWrite },
            .reset = I8080DeviceFileInOutReset,
            .shutdown = I8080DeviceFileInOutShutdown
        };
const I8080DevicePtr I8080DeviceFileInOut = &__I8080DeviceFileInOut;
