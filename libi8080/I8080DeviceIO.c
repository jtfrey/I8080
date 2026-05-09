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
    
    while ( (i < devbus->n_input) && (i < devbus->n_output) ) {
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
        if ( same_device && devbus->input_devs[i].device_ptr->reset )
            devbus->input_devs[i].device_ptr->reset(devbus->input_devs[i].device_ptr, devbus->input_devs[i].context);
        i++;
    }
    while ( i < devbus->n_input ) {
        if ( devbus->input_devs[i].device_ptr ) {
            devbus->input_devs[i].device_ptr->input.bytes_in = 0;
            if ( devbus->input_devs[i].device_ptr->reset )
                devbus->input_devs[i].device_ptr->reset(devbus->input_devs[i].device_ptr, devbus->input_devs[i].context);
        }
        i++;
    }
    while ( i < devbus->n_output ) {
        if ( devbus->output_devs[i].device_ptr ) {
            devbus->output_devs[i].device_ptr->output.bytes_out = 0;
            if ( devbus->output_devs[i].device_ptr->reset )
                devbus->output_devs[i].device_ptr->reset(devbus->output_devs[i].device_ptr, devbus->output_devs[i].context);
        }
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
    
    while ( (i < devbus->n_input) && (i < devbus->n_output) ) {
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
        if ( same_device && devbus->input_devs[i].device_ptr->shutdown )
            devbus->input_devs[i].device_ptr->shutdown(devbus->input_devs[i].device_ptr, devbus->input_devs[i].context);
        i++;
    }
    while ( i < devbus->n_input ) {
        if ( devbus->input_devs[i].device_ptr ) {
            devbus->input_devs[i].device_ptr->input.bytes_in = 0;
            if ( devbus->input_devs[i].device_ptr->shutdown )
                devbus->input_devs[i].device_ptr->shutdown(devbus->input_devs[i].device_ptr, devbus->input_devs[i].context);
        }
        i++;
    }
    while ( i < devbus->n_output ) {
        if ( devbus->output_devs[i].device_ptr ) {
            devbus->output_devs[i].device_ptr->output.bytes_out = 0;
            if ( devbus->output_devs[i].device_ptr->shutdown )
                devbus->output_devs[i].device_ptr->shutdown(devbus->output_devs[i].device_ptr, devbus->output_devs[i].context);
        }
        i++;
    }
}

//

bool
I8080DevBusRegisterDevice(
    I8080DevBusRef  devbus,
    I8080Device_t   *dev,
    const void      *context
)
{
    bool            ok = true, did_add_input_dev = false;
    
    if ( dev->device_mode & kI8080DeviceModeInput ) {
        if ( devbus->n_input < 0xFF ) {
            dev->input.device_id = devbus->n_input++;
            dev->input.bytes_in = 0LL;
            devbus->input_devs[dev->input.device_id].device_ptr = dev;
            devbus->input_devs[dev->input.device_id].context = context;
            did_add_input_dev = true;
            INFO("Input device \"%s\" registered as 0x%02hhX with device bus %p",
                dev->device_name ? dev->device_name : "<UNKNOWN>",
                dev->input.device_id,
                devbus);
        } else {
            ERROR("Attempted to register more than 256 input devices");
            ok = false;
        }
    }
    if ( ok && (dev->device_mode & kI8080DeviceModeOutput) ) {
        if ( devbus->n_output < 0xFF ) {
            dev->output.device_id = devbus->n_output++;
            dev->output.bytes_out = 0LL;
            devbus->output_devs[dev->output.device_id].device_ptr = dev;
            devbus->output_devs[dev->output.device_id].context = context;
            INFO("Output device \"%s\" registered as 0x%02hhX with device bus %p",
                dev->device_name ? dev->device_name : "<UNKNOWN>",
                dev->output.device_id,
                devbus);
        } else {
            ERROR("Attempted to register more than 256 output devices\n");
            ok = false;
            // Drop the input registration is it was made:
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
    
    if ( (dev_id < devbus->n_input) && (devbus->input_devs[dev_id].device_ptr) ) {
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
   if ( (dev_id < devbus->n_output) && (devbus->output_devs[dev_id].device_ptr) ) {
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
    
    while ( (i < devbus->n_input) && (i < devbus->n_output) ) {
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
    while ( i < devbus->n_input ) {
        if ( devbus->input_devs[i].device_ptr ) {
            fprintf(stream, "I8080Device[%02X] [←%08lX|         ] BYTES  \"%s\"\n",
                    i, devbus->input_devs[i].device_ptr->input.bytes_in,
                    devbus->input_devs[i].device_ptr->device_name);
        }
        i++;
    }
    while ( i < devbus->n_output ) {
        if ( devbus->output_devs[i].device_ptr ) {
            fprintf(stream, "I8080Device[%02X] [         |→%08lX] BYTES  \"%s\"\n",
                    i, devbus->output_devs[i].device_ptr->output.bytes_out,
                    devbus->output_devs[i].device_ptr->device_name);
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
    
    if ( stdout_context && (stdout_context->options & kI8080DeviceStdOutputOptsClearScreenOnReset) ) {
        printf("\033[H\033[2J");fflush(stdout);
    }
}

static
void
I8080DeviceStdOutputShutdown(
    I8080DevicePtr  dev,
    const void      *context
)
{
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
