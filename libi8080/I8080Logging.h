/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 logging API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080LOGGING_H__
#define __I8080LOGGING_H__

#include "I8080Config.h"

/**
 * Logging level
 * Messages are logged at a specific severity level; the API will
 * filter-out any message of higher level than the set max level.
 */
typedef enum {
    kI8080LoggingLevelCritical = -10,   /*!< An unfilterable level */
    kI8080LoggingLevelError     = 0,    /*!< Generally-severe errors */
    kI8080LoggingLevelWarning   = 10,   /*!< Errors that are not deal-breakers */
    kI8080LoggingLevelInfo      = 20,   /*!< Informative information… */
    kI8080LoggingLevelDebug     = 50    /*!< …that's not excessive junk for the developer */
} I8080LoggingLevel_t;

/**
 * Logging format
 * The messages produced by the logging facility have a number of
 * fields that can be turned on and off.
 *
 * The default is to only print the level name and message.
 */
typedef enum {
    kI8080LoggingFormatTimestamp    = 0b1,          /*!< Show a leading timestamp… */
    kI8080LoggingFormatPID          = 0b10,         /*!< …followed by the pid… */
    kI8080LoggingFormatLevelNumber  = 0b100,        /*!< …then the level number… */
    kI8080LoggingFormatLevelName    = 0b1000,       /*!< …and the level name… */
    kI8080LoggingFormatMessage      = 0b10000,      /*!< …before the actual message. */
    kI8080LoggingFormatSourceLoc    = 0b100000,     /*!< Tack-on the source file:line */
    kI8080LoggingFormatSourceFunc   = 0b1000000,    /*!< Tack-on the function that logged the message */
    
    kI8080LoggingFormatDefault      = kI8080LoggingFormatLevelName | kI8080LoggingFormatMessage,
    kI8080LoggingFormatDetailed     = kI8080LoggingFormatTimestamp | kI8080LoggingFormatLevelName | kI8080LoggingFormatMessage |
                                      kI8080LoggingFormatSourceFunc,
    
    kI8080LoggingFormatAll          = 0b1111111     /*!< convenient way to specify all fields */
} I8080LoggingFormat_t;

/**
 * Get the max logging level
 * Returns the API's current maximum logging level; all messages of
 * levels greater than this value will be suppressed.
 * @return              the current maximum logging level
 */
I8080LoggingLevel_t I8080LoggingGetMaxLevel(void);

/**
 * Set the max logging level
 * Sets the API's maximum logging level; all messages of levels greater
 * than this value will be suppressed.
 * @param level         the new max level
 */
void I8080LoggingSetMaxLevel(I8080LoggingLevel_t level);

/**
 * Get the logging format
 * Returns the API's current logging format.
 */
I8080LoggingFormat_t I8080LoggingGetFormat(void);

/**
 * Set the logging format
 * Sets the API's logging format.
 * @param format        the bitvector of desired fields
 */
void I8080LoggingSetFormat(I8080LoggingFormat_t format);

/**
 * Set the logging stream
 * Any subsequent logging messages will be written to \p stream.  If
 * the stream is altered again at any point, the value of \p should_close
 * determines whether or not this API closes the stream.
 *
 * By default the API logs to stderr.
 * @param stream        the file stream to which messages should be
 *                      logged
 * @param should_close  close the stream if the stream is changed again
 */
void I8080LoggingSetStream(FILE *stream, bool should_close);

/**
 * Set the logging stream by opening a file
 * Convenience function that attempts to open the file at \p filepath
 * for writing (or append, as dictated by \p should_append).  If
 * successful, \ref I8080LoggingSetStream() is called with the open
 * FILE* and \p should_close of true.
 * @param filepath          the file to which messages should be logged
 * @param should_append     if true, open the file in append mode rather
 *                          than write
 * @return                  boolean true if the file was opened
 *                          successfully
 */
bool I8080LoggingSetFile(const char *filepath, bool should_append);

/**
 * Log a message
 * Use the \ref printf() \p fmt and any additional arguments to create a string.
 * Each line of the string will be output to the logging facility if the current
 * maximum logging level is at least as high as \p level.
 *
 * The \p src_file, \p fn_name, and \p line_no indicate from where in the source
 * code the message was logged; these are easily handled by means of preprocessor
 * macros in the convenience methods (\ref ERROR() et al.).
 * @param level             the level at which this message should be logged
 * @param src_file          optional:  the source file associated with this
 *                          message
 * @param line_no           optional:  the line number at which this message
 *                          was logged
 * @param fn_name           the name of the function that logged this message
 * @param fmt               a \ref printf() format string
 */
void I8080LoggingLogMessage(I8080LoggingLevel_t level, const char *src_file, int line_no, const char *fn_name, const char *fmt, ...);

/**
 * Log a critical message
 * Helper macro that logs a critical-level message.  The file, line number,
 * and function name are filled in with preprocessor macros.
 */
#define CRITICAL(FMT, ...) I8080LoggingLogMessage(kI8080LoggingLevelCritical, __FILE__, __LINE__, LOGGING_FUNC, FMT, ##__VA_ARGS__)

/**
 * Log an error message
 * Helper macro that logs an error-level message.  The file, line number,
 * and function name are filled in with preprocessor macros.
 */
#define ERROR(FMT, ...) I8080LoggingLogMessage(kI8080LoggingLevelError, __FILE__, __LINE__, LOGGING_FUNC, FMT, ##__VA_ARGS__)

/**
 * Log a warning message
 * Helper macro that logs a warning-level message.  The file, line number,
 * and function name are filled in with preprocessor macros.
 */
#define WARNING(FMT, ...) I8080LoggingLogMessage(kI8080LoggingLevelWarning, __FILE__, __LINE__, LOGGING_FUNC, FMT, ##__VA_ARGS__)

/**
 * Log an ino message
 * Helper macro that logs an info-level message.  The file, line number,
 * and function name are filled in with preprocessor macros.
 */
#define INFO(FMT, ...) I8080LoggingLogMessage(kI8080LoggingLevelInfo, __FILE__, __LINE__, LOGGING_FUNC, FMT, ##__VA_ARGS__)

/**
 * Log a debug message
 * Helper macro that logs a debug-level message.  The file, line number,
 * and function name are filled in with preprocessor macros.
 */
#ifdef I8080_STRIP_DEBUGGING
#   define DEBUG(FMT, ...)
#else
#   define DEBUG(FMT, ...) I8080LoggingLogMessage(kI8080LoggingLevelDebug, __FILE__, __LINE__, LOGGING_FUNC, FMT, ##__VA_ARGS__)
#endif

#endif /* __I8080LOGGING_H__ */
