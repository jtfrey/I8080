/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 device i/o API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080DEVICEIO_H__
#define __I8080DEVICEIO_H__

#include "I8080Config.h"

/**
 * I/O device id
 * The I8080 allows for up to 256 input and 256 output
 * devices addressable by the CPU.
 */
typedef uint8_t I8080DevId;

/**
 * I/O device type
 * I/O devices in this implementation can function in input, output, or
 * both directions.
 */
typedef enum __attribute__((packed)) {
    kI8080DeviceModeInput       = 0b01,     /*!< the device can provide input */
    kI8080DeviceModeOutput      = 0b10,     /*!< the device can accept output */
    kI8080DeviceModeInputOutput = 0b11      /*!< the device provides input and accepts output */
} I8080DeviceMode;

/**
 * Opaque pointer to a device definition
 * This is a forward declaration primarily for the sake of the callback
 * function definitions.
 */
typedef struct I8080Device * I8080DevicePtr;

/**
 * Input device read function
 * Read a byte of input from the device.
 * @param dev       pointer to the device definition
 * @param context   the user-defined pointer associated with the device when it
 *                  was registered on a bus.
 * @return          the byte read from the device
 */
typedef uint8_t (*I8080DeviceIOByteReadCallback)(I8080DevicePtr dev, const void *context);

/**
 * Output device write function
 * Write a byte of output to the device.
 * @param dev       pointer to the device definition
 * @param byte      the byte to be written to the device
 * @param context   the user-defined pointer associated with the device when it
 *                  was registered on a bus.
 */
typedef void (*I8080DeviceIOByteWriteCallback)(I8080DevicePtr dev, uint8_t byte, const void *context);

/**
 * Device reset function
 * Cause the device to react to a system reset.
 * @param dev       pointer to the device definition
 * @param context   the user-defined pointer associated with the device when it
 *                  was registered on a bus.
 */
typedef void (*I8080DeviceIOReset)(I8080DevicePtr dev, const void *context);

/**
 * Device shutdown function
 * Cause the device to react to a system shutdown.
 * @param dev       pointer to the device definition
 * @param context   the user-defined pointer associated with the device when it
 *                  was registered on a bus.
 */
typedef void (*I8080DeviceIOShutdown)(I8080DevicePtr dev, const void *context);

/**
 * I/O device definition
 * Every device must have a data structure of this type to register
 * with a device bus.  The device bus will fill-in the \p device_id
 * fields at that time.
 *
 * The \p device_name is an arbitrary identifier that can aid in
 * debugging.
 *
 * If the device is capable of input (as indicated by the \p device_mode)
 * a pointer to the \p read callback should be set.
 *
 * If the device is capable of output (as indicated by the \p device_mode)
 * a pointer to the \p write callback should be set.
 */
typedef struct I8080Device {
    const char                          *device_name;   /*!< C string that identifies the device */
    I8080DeviceMode                     device_mode;    /*!< indicates which i/o modes the device provides */
    struct {
        I8080DevId                      device_id;      /*!< device id assigned when this device is registered
                                                             on a bus as an input device */
        I8080DeviceIOByteReadCallback   read;           /*!< function that reads a byte from the device */
        size_t                          bytes_in;       /*!< counter for the number of bytes read from this device */
    } input;
    struct {
        I8080DevId                      device_id;      /*!< device id assigned when this device is registered
                                                             on a bus as an output device */
        I8080DeviceIOByteWriteCallback  write;          /*!< function that writes a byte to the device */
        size_t                          bytes_out;      /*!< counter for the number of bytes read from this device */
    } output;
    I8080DeviceIOReset                  reset;          /*!< function that handles a device reset */
    I8080DeviceIOShutdown               shutdown;       /*!< function that handles device shutdown */
} I8080Device_t;

/**
 * Reference to an I/O bus
 * I/O devices are collectively registered with an instance of the
 * I8080DevBus pseudo-class.  The internal organization of the class
 * is private, so an opaque pointer is used to represent it.
 */
typedef struct I8080DevBus * I8080DevBusRef;

/**
 * Create a new I/O bus
 * @return          an empty, newly-initialized I/O bus
 */
I8080DevBusRef I8080DevBusCreate(void);

/**
 * Destroy an I/O bus
 * All registered devices have their device_ids zeroed and \p devbus
 * is deallocated.
 * @param devbus        the I/O bus to destroy
 */
void I8080DevBusDestroy(I8080DevBusRef devbus);

/**
 * Reset an I/O bus
 * All registered devices are reset and their byte-counters are zeroed.
 * @param devbus        the I/O bus to reset
 */
void I8080DevBusReset(I8080DevBusRef devbus);

/**
 * Shutdown an I/O bus
 * All registered devices with a \ref I8080DeviceIOShutdown() function
 * associated with them have that function called.
 *
 * The bus can be subsequently "turned on" by calling \ref I8080DevBusReset().
 * @param devbus        the I/O bus to shutdown
 */
void I8080DevBusShutdown(I8080DevBusRef devbus);

/**
 * Desired device id
 * When registering a device on the bus, a specific id can be requested or
 * the bus can be allowed to assign an id.
 */
typedef enum {
    kI8080DevBusRegisterIdNextAvail = -1,       /*!< the bus should assign an available id to the device */
    kI8080DevBusRegisterIdMask        = 0xFF    /*!< values in this range are requestable */
} I8080DevBusRegisterId;

/**
 * Register a device on the bus
 * The \p dev is registered with \p devbus under the modes it indicates.
 * The \p context pointer is associated with the device and will be passed
 * to its callbacks when invoked.
 * @param devbus        the I/O bus on which to register the device
 * @param dev           pointer to a device definition
 * @param context       user-defined pointer that is associated with the
 *                      device and passed to its callbacks when invoked
 */
bool I8080DevBusRegisterDevice(I8080DevBusRef devbus, I8080DevBusRegisterId req_id, I8080DevicePtr dev, const void *context);

/**
 * Unregister a device from the bus
 * The \p dev is removed from \p devbus under the modes it indicates.
 * The device's device id fields are zeroed.
 *
 * If \p dev was the last-registered input or output device, its
 * device id will become available for reuse by the next device
 * that is registered.
 * @param devbus        the I/O bus on which the device is registered
 * @param dev           pointer to a device definition
 */
void I8080DevBusUnregisterDevice(I8080DevBusRef devbus, I8080DevicePtr dev);

/**
 * Read a byte from a device on the bus
 * Attempt to read a byte from device \p dev_id on \p devbus.  If no device
 * exists with that id, a zero byte is returned and a warning will be printed
 * to stderr.
 * @param devbus        the I/O bus on which the device is registered
 * @param dev_id        the device to read
 * @return              the byte read or zero on error
 */
uint8_t I8080DevBusReadDevice(I8080DevBusRef devbus, I8080DevId dev_id);

/**
 * Write a byte to a device on the bus
 * Attempt to write a byte to device \p dev_id on \p devbus.  If no device
 * exists with that id, a warning will be printed to stderr.
 * @param devbus        the I/O bus on which the device is registered
 * @param dev_id        the device to write
 * @param byte          the byte to write
 */
void I8080DevBusWriteDevice(I8080DevBusRef devbus, I8080DevId dev_id, uint8_t byte);

/**
 * Write a summary of the device bus to a FILE stream
 * Writes a summary of the device bus \p devbus to the FILE stream
 * \p stream.
 * @param stream        the stream to which to write
 * @param devbus        the I/O bus
 */
void I8080DevBusPrint(FILE *stream, I8080DevBusRef devbus);

/** 
 * I/O device definition that interfaces with stdin
 * Pointer to a device definition that reads bytes from stdin.  The consumer
 * should copy the contents of this definition to their own \ref I8080Device_t
 * data structure rather than registering \p I8080DeviceStdInput directly on
 * a bus.
 */
extern const I8080DevicePtr I8080DeviceStdInput;

#include <termios.h>

/**
 * Stdin device options
 * In conjunction with a \ref I8080DeviceStdInputContext_t structure, these
 * option bits control how the device will alter the terminal behavior.
 *
 * Do NOT set any bits outside those defined here!  Other bits may be
 * used internally by the device.
 */
typedef enum {
    kI8080DeviceStdInputOptsSetNonCanonicalMode = 0b1,      /*!< respond to input of one character at a time */
    kI8080DeviceStdInputOptsDisableEcho         = 0b10,     /*!< do not echo typed characters */
    
    kI8080DeviceStdInputOptsAll                 = 0b111     /*!< convenience value that contains all options set */
} I8080DeviceStdInputOpts_t;

/**
 * Stdin device context data
 * Allocate one of these data structures and set the \p options field to
 * alter the terminal behavior.  Pass a pointer to the resulting data structure
 * to \ref I8080DevBusRegisterDevice() in conjunction with a \ref I8080DeviceStdInput
 * instance and the device will automatically adopt the desired behavioral changes
 * when it comes online (reset), and restore the original terminal behaviors once it
 * is shutdown.
 *
 * The caller should NOT make changes to this data structure once passed to the
 * \ref I8080DevBusRegisterDevice() function.  The device references the
 * original copy!
 */
typedef struct {
    I8080DeviceStdInputOpts_t   options;        /*!< options bitvector, see \ref I8080DeviceStdInputOpts_t */
    struct termios              saved_state;    /*!< to hold a copy of the original terminal state */
} I8080DeviceStdInputContext_t;

/** 
 * I/O device definition that interfaces with stdout
 * Pointer to a device definition that writes bytes to stdout.  The consumer
 * should copy the contents of this definition to their own \ref I8080Device_t
 * data structure rather than registering \p I8080DeviceStdInput directly on
 * a bus.
 */
extern const I8080DevicePtr I8080DeviceStdOutput;

/**
 * Stdout device options
 * In conjunction with a \ref I8080DeviceStdOutputContext_t structure, these
 * option bits control how the device will alter the terminal behavior.
 *
 * Do NOT set any bits outside those defined here!  Other bits may be
 * used internally by the device.
 */
typedef enum {
    kI8080DeviceStdOutputOptsClearScreenOnReset = 0b1,      /*!< respond to input of one character at a time */
#ifdef HAS_TERMIOS_OLCUC
    kI8080DeviceStdOutputOptsForceUppercase     = 0b10,     /*!< force all output to uppercase alphabet */
    kI8080DeviceStdOutputOptsAll                = 0b11      /*!< convenience value that contains all options set */
#else
    kI8080DeviceStdOutputOptsAll                = 0b1       /*!< convenience value that contains all options set */
#endif
} I8080DeviceStdOutputOpts_t;

/**
 * Stdout device context data
 * Allocate one of these data structures and set the \p options field to
 * alter the terminal behavior.  Pass a pointer to the resulting data structure
 * to \ref I8080DevBusRegisterDevice() in conjunction with a \ref I8080DeviceStdOutput
 * instance and the device will automatically adopt the desired behavioral changes
 * when it comes online (reset), and restore the original terminal behaviors once it
 * is shutdown.
 *
 * The caller should NOT make changes to this data structure once passed to the
 * \ref I8080DevBusRegisterDevice() function.  The device references the
 * original copy!
 */
typedef struct {
    I8080DeviceStdOutputOpts_t  options;        /*!< options bitvector, see \ref I8080DeviceStdOutputOpts_t */
    struct termios              saved_state;    /*!< to hold a copy of the original terminal state */
} I8080DeviceStdOutputContext_t;

/** 
 * I/O device definition that interfaces with stderr
 * Pointer to a device definition that writes bytes to stderr.  The consumer
 * should copy the contents of this definition to their own \ref I8080Device_t
 * data structure rather than registering \p I8080DeviceStdInput directly on
 * a bus.
 */
extern const I8080DevicePtr I8080DeviceStdError;

/** 
 * I/O device definition that combines stdin/stdout
 * Pointer to a device definition that behaves as a combined input and
 * output device.  Behind the scenes, the input is provided by stdin and
 * the output by stdout.  The consumer should copy the contents of this
 * definition to their own \ref I8080Device_t data structure rather than
 * registering \p I8080DeviceStdInput directly on a bus.
 */
extern const I8080DevicePtr I8080DeviceTTY;

/**
 * TTY device context data
 * Pairs the context data of \ref I8080DeviceStdInputOpts_t with that of
 * \ref I8080DeviceStdOutputOpts_t since the TTY embodies both of those
 * devices.  Allocate one of these data structures and set the \p stdin_context
 * and \p stdout_context structure fields to alter terminal behavior.  Pass a pointer
 * to the resulting data structure to \ref I8080DevBusRegisterDevice() in conjunction
 * with a \ref I8080DeviceTTY instance and the device will automatically adopt the
 * desired behavioral changes when it comes online (reset), and restore the original
 * terminal behaviors once it is shutdown.
 *
 * The caller should NOT make changes to this data structure once passed to the
 * \ref I8080DevBusRegisterDevice() function.  The device references the
 * original copy!
 */
typedef struct {
    I8080DeviceStdInputContext_t    stdin_context;
    I8080DeviceStdOutputContext_t   stdout_context;
} I8080DeviceTTYContext_t;



/** 
 * I/O device definition for input from a file
 * Pointer to a device definition that reads bytes from a file.
 * The file is communicated to the device via a
 * \ref I8080DeviceFileContext_t context provided at registration.  The
 * consumer should copy the contents of this definition to their own
 * \ref I8080Device_t data structure rather than registering \p I8080DeviceFileIn
 * directly on a bus.
 */
extern const I8080DevicePtr I8080DeviceFileIn;

/** 
 * I/O device definition for output to a file
 * Pointer to a device definition that writes bytes to a file.
 * The file is communicated to the device via a
 * \ref I8080DeviceFileContext_t context provided at registration.  The
 * consumer should copy the contents of this definition to their own
 * \ref I8080Device_t data structure rather than registering \p I8080DeviceFileOut
 * directly on a bus.
 */
extern const I8080DevicePtr I8080DeviceFileOut;

/** 
 * I/O device definition for input/output from/to a file
 * Pointer to a device definition that reads/writes bytes from/to a file.
 * The file is communicated to the device via a
 * \ref I8080DeviceFileContext_t context provided at registration.  The
 * consumer should copy the contents of this definition to their own
 * \ref I8080Device_t data structure rather than registering \p I8080DeviceFileInOut
 * directly on a bus.
 */
extern const I8080DevicePtr I8080DeviceFileInOut;

/**
 * I8080DeviceFile variant
 * The \ref I8080DeviceFileContext_t structure embodies two different
 * configuration methods:  a filepath and mode string or an
 * already-open FILE pointer.  This enumeration selects which is
 * chosen.
 */
typedef enum {
    kI8080DeviceFileVariantPath,        /*!< I8080DeviceFile using filepath and mode */ 
    kI8080DeviceFileVariantFILEPtr      /*!< I8080DeviceFile using FILE pointer */
} I8080DeviceFileVariant_t;

/**
 * File device context data
 * Provides configurational parameters to a \ref I8080DeviceFileIn,
 * \ref I8080DeviceFileOut, or \ref I8080DeviceFileInOut device.
 *
 * The caller should NOT make changes to this data structure once passed to the
 * \ref I8080DevBusRegisterDevice() function.  The device references the
 * original copy!
 */
typedef struct {
    I8080DeviceFileVariant_t    variant;        /*!< the I8080DeviceFile variant */ 
    union {
        struct {
            const char          *filepath;      /*!< path to the file to be opened */
            const char          *mode;          /*!< \ref fopen() mode in which to open the file */
            FILE                *fptr;          /*!< set by the device once the file is open */
        } path;
        struct {
            bool                should_close_on_shutdown;   /*!< true = close the file on first device shutdown;
                                                                 the file will no longer be open or usable if the
                                                                 device is reset */
            FILE                *fptr;          /*!< the externally-opened FILE pointer to associate with this
                                                     device */
        } fileptr;
    };
} I8080DeviceFileContext_t;

#endif /* __I8080DEVICEIO_H__ */
