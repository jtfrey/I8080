/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 Game pad.
 */
 
#include "I8080DPad.h"
#include "I8080Logging.h"

#ifdef I8080_PPU_OSX_KEYCHECKS
#   include <ApplicationServices/ApplicationServices.h>
#   include <Carbon/Carbon.h>
#   include <pthread.h>
static pthread_mutex_t      I8080DeviceDPadLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t            I8080DeviceDPadMonitorThread;
static bool                 I8080DeviceDPadIsMonitorLaunched = false;
static CFMachPortRef        I8080DeviceDPadEventTap;
static CFRunLoopRef         I8080DeviceDPadRunloop;
static CFRunLoopSourceRef   I8080DeviceDPadRunloopSource;
static uint8_t              I8080DeviceDPadReading = 0x00;

//

CGEventRef
__CGWControlsKeyEventCallback(
    CGEventTapProxy     proxy,
    CGEventType         type, 
    CGEventRef          event,
    void                *refcon
)
{
    CGEventRef          rc = event;
    int                 keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    
    if ( pthread_mutex_lock(&I8080DeviceDPadLock) == 0 ) {
        uint8_t     mask = 0x00;
        
        switch ( keyCode ) {
            case kVK_ANSI_A:
            case kVK_Option:
            case kVK_RightOption:
            case kVK_Space:
                mask = kI8080DPadBitsA;
                break;
            case kVK_ANSI_S:
            case kVK_Command:
            case kVK_RightCommand:
                mask = kI8080DPadBitsB;
                break;
            case kVK_ANSI_Q:
            case kVK_Shift:
            case kVK_RightShift:
                mask = kI8080DPadBitsSelect;
                break;
            case kVK_ANSI_W:
            case kVK_Return:
                mask = kI8080DPadBitsStart;
                break;
            case kVK_ANSI_I:
            case kVK_UpArrow:
                mask = kI8080DPadBitsUp;
                break;
            case kVK_ANSI_K:
            case kVK_DownArrow:
                mask = kI8080DPadBitsDown;
                break;
            case kVK_ANSI_J:
            case kVK_LeftArrow:
                mask = kI8080DPadBitsLeft;
                break;
            case kVK_ANSI_L:
            case kVK_RightArrow:
                mask = kI8080DPadBitsRight;
                break;
        }
        if ( mask ) {
            switch ( type ) {
                case kCGEventFlagsChanged:  I8080DeviceDPadReading ^=  mask; break;
                case kCGEventKeyDown:       I8080DeviceDPadReading |=  mask; break;
                case kCGEventKeyUp:         I8080DeviceDPadReading &= ~mask; break;
            }
            rc = NULL;
        }
        pthread_mutex_unlock(&I8080DeviceDPadLock);
    }
    return rc;
}

void*
I8080DeviceDPadRunloopThread(
    void        *context
)
{
    pthread_mutex_lock(&I8080DeviceDPadLock);
    I8080DeviceDPadRunloop = CFRunLoopGetCurrent();
    I8080DeviceDPadEventTap = CGEventTapCreate(
                                        kCGAnnotatedSessionEventTap,
                                        kCGHeadInsertEventTap,
                                        kCGEventTapOptionListenOnly,                // Passive listener
                                        CGEventMaskBit(kCGEventKeyDown) |           // Detect key down...
                                            CGEventMaskBit(kCGEventKeyUp) |         // ...detect key up ...
                                            CGEventMaskBit(kCGEventFlagsChanged),   // ...detect modifiers
                                        __CGWControlsKeyEventCallback,
                                        NULL
                                    );  
    if ( I8080DeviceDPadEventTap ) {
        I8080DeviceDPadRunloopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, I8080DeviceDPadEventTap, 0);
        if ( I8080DeviceDPadRunloopSource ) {
            CFRunLoopAddSource(I8080DeviceDPadRunloop, I8080DeviceDPadRunloopSource, kCFRunLoopCommonModes);
            CGEventTapEnable(I8080DeviceDPadEventTap, true);
            I8080DeviceDPadIsMonitorLaunched = true;
            pthread_mutex_unlock(&I8080DeviceDPadLock);
            CFRunLoopRun();
            pthread_mutex_lock(&I8080DeviceDPadLock);
            CGEventTapEnable(I8080DeviceDPadEventTap, false);
        }
        CFRunLoopSourceInvalidate(I8080DeviceDPadRunloopSource);
        CFRelease(I8080DeviceDPadRunloopSource);
        CFRelease(I8080DeviceDPadEventTap);
    } else {
        CRITICAL("Unable to create keyboard event tap for D-pad controller.");
        exit(0);
    }
    I8080DeviceDPadIsMonitorLaunched = false;
    pthread_mutex_unlock(&I8080DeviceDPadLock);
    return NULL;
}

//

uint8_t
I8080DeviceDPadRead(
    I8080DevicePtr  dev,
    const void      *context
)
{
    uint8_t         out_reading;
    
    pthread_mutex_lock(&I8080DeviceDPadLock);
    out_reading = I8080DeviceDPadReading;
    pthread_mutex_unlock(&I8080DeviceDPadLock);
    DEBUG("D-pad [ABsSUDLR] = [%u%u%u%u%u%u%u%u]",
        (out_reading & kI8080DPadBitsA) != 0,
        (out_reading & kI8080DPadBitsB) != 0,
        (out_reading & kI8080DPadBitsSelect) != 0,
        (out_reading & kI8080DPadBitsStart) != 0,
        (out_reading & kI8080DPadBitsUp) != 0,
        (out_reading & kI8080DPadBitsDown) != 0,
        (out_reading & kI8080DPadBitsLeft) != 0,
        (out_reading & kI8080DPadBitsRight) != 0);
    return out_reading;
}

//

void
I8080DeviceDPadReset(
    I8080DevicePtr  dev,
    const void      *context
)
{
    if ( ! I8080DeviceDPadIsMonitorLaunched ) {
        CFDictionaryRef     trust = CFDictionaryCreate(kCFAllocatorDefault,
                                            (const void**)&kAXTrustedCheckOptionPrompt,
                                            (const void**)&kCFBooleanTrue,
                                            1,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
        if ( ! AXIsProcessTrustedWithOptions(trust) ) {
            CRITICAL("Input monitoring must be enabled on the terminal program.");
            exit(0);
        }
        CFRelease(trust);
        nodelay(stdscr, FALSE);
        noecho();
        pthread_create(&I8080DeviceDPadMonitorThread, NULL, I8080DeviceDPadRunloopThread, NULL);
    }
}

//

void
I8080DeviceDPadShutdown(
    I8080DevicePtr  dev,
    const void      *context
)
{
    if ( I8080DeviceDPadIsMonitorLaunched ) {
        CFRunLoopStop(I8080DeviceDPadRunloop);
        pthread_join(I8080DeviceDPadMonitorThread, NULL);
        // Consume all buffered input from the keyboard...
        halfdelay(1);
        while ( getch() != ERR );
    }
}

//

I8080Device_t __I8080DeviceDPadIn = {
            .device_name = "game-dpad",
            .device_mode = kI8080DeviceModeInput,
            .input = {
                .read = I8080DeviceDPadRead },
            .output = {
                .write = NULL },
            .reset = I8080DeviceDPadReset,
            .shutdown = I8080DeviceDPadShutdown,
            .name = NULL
        };

#else

uint8_t
I8080DeviceDPadRead(
    I8080DevicePtr  dev,
    const void      *context
)
{
    uint8_t         dpad_read;
    int             keycode;
    
    while ( (keycode = getch()) != ERR ) {
        switch ( keycode ) {
            case KEY_UP:
            case 'I':
            case 'i':
                dpad_read |= kI8080DPadBitsUp;
                break;
            case KEY_LEFT:
            case 'J':
            case 'j':
                dpad_read |= kI8080DPadBitsLeft;
                break;
            case KEY_DOWN:
            case 'K':
            case 'k':
                dpad_read |= kI8080DPadBitsDown;
                break;
            case KEY_RIGHT:
            case 'L':
            case 'l':
                dpad_read |= kI8080DPadBitsRight;
                break;
            case 'A':
            case 'a':
            case ' ':
                dpad_read |= kI8080DPadBitsA;
                break;
            case 'S':
            case 's':
            case 'V':
            case 'v':
            case 'B':
            case 'b':
                dpad_read |= kI8080DPadBitsB;
                break;
            case 'Q':
            case 'q':
                dpad_read |= kI8080DPadBitsSelect;
                break;
            case 'W':
            case 'w':
            case '\n':
            case '\r':
            case KEY_ENTER:
                dpad_read |= kI8080DPadBitsStart;
                break;
        }
    }
    return dpad_read;
}

//

void
I8080DeviceDPadReset(
    I8080DevicePtr  dev,
    const void      *context
)
{
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    noecho();
}

//

void
I8080DeviceDPadShutdown(
    I8080DevicePtr  dev,
    const void      *context
)
{
    keypad(stdscr, FALSE);
    nodelay(stdscr, FALSE);
    echo();
}

//

I8080Device_t __I8080DeviceDPadIn = {
            .device_name = "game-dpad",
            .device_mode = kI8080DeviceModeInput,
            .input = {
                .read = I8080DeviceDPadRead },
            .output = {
                .write = NULL },
            .reset = I8080DeviceDPadReset,
            .shutdown = I8080DeviceDPadShutdown,
            .name = NULL
        };

#endif

const I8080DevicePtr I8080DeviceDPadIn = &__I8080DeviceDPadIn;
