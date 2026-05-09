/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 logging API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */

#include "I8080Logging.h"
#include <time.h>

//

typedef struct {
    I8080LoggingLevel_t     level_num;
    const char              *level_name;
} I8080LoggingLevelName_t;

static I8080LoggingLevelName_t I8080LoggingLevelNames[] = {
            { kI8080LoggingLevelCritical, "CRITICAL" },
            { kI8080LoggingLevelError, "ERROR" },
            { kI8080LoggingLevelWarning, "WARNING" },
            { kI8080LoggingLevelInfo, "INFO" },
            { kI8080LoggingLevelDebug, "DEBUG" },
            { 0, NULL }
        };

static inline
const char*
I8080LoggingLevelNameForLevel(
    I8080LoggingLevel_t level
)
{
    int         i = 0;
    
    while ( I8080LoggingLevelNames[i].level_name && I8080LoggingLevelNames[i].level_num < level ) i++;
    return I8080LoggingLevelNames[i].level_name ? I8080LoggingLevelNames[i].level_name : I8080LoggingLevelNames[i-1].level_name;
}

//

static I8080LoggingLevel_t __loglevel = kI8080LoggingLevelError;

I8080LoggingLevel_t
I8080LoggingGetMaxLevel(void)
{
    return __loglevel;
}

void
I8080LoggingSetMaxLevel(
    I8080LoggingLevel_t level
)
{
    if ( level >= -1 ) __loglevel = level;
}

//

static I8080LoggingFormat_t __logformat = kI8080LoggingFormatDefault;

I8080LoggingFormat_t
I8080LoggingGetFormat(void)
{
    return __logformat;
}

void
I8080LoggingSetFormat(
    I8080LoggingFormat_t    format
)
{
    __logformat = format & kI8080LoggingFormatAll;
}

//

static FILE *__logstream = NULL;
static bool __should_close_logstream = false;

void
I8080LoggingSetStream(
    FILE    *stream,
    bool    should_close
)
{
    if ( stream != __logstream ) {
        if ( __should_close_logstream ) fclose(__logstream);
        __logstream = stream;
        __should_close_logstream = should_close;
    }
}

bool
I8080LoggingSetFile(
    const char  *filepath,
    bool        should_append
)
{
    FILE        *stream = fopen(filepath, should_append ? "a" : "w");
    
    if ( stream ) {
        I8080LoggingSetStream(stream, true);
        return true;
    }
    return false;
}

//

void
I8080LoggingLogMessage(
    I8080LoggingLevel_t     level,
    const char              *src_file,
    int                     line_no,
    const char              *fn_name,
    const char              *fmt,
    ...
)
{
    if ( level <= __loglevel ) {
        FILE                *logstream = __logstream ? __logstream : stderr;
        
        if ( __logformat & kI8080LoggingFormatMessage ) {
            static char     log_line[2048];
            size_t          log_line_len = 0, log_line_size = sizeof(log_line);
            int             i;
            va_list         argv;
            
            va_start(argv, fmt);
            log_line_len = vsnprintf(&log_line[log_line_len], log_line_size, fmt, argv);
            va_end(argv);
            
            i = 0;
            while ( (i < log_line_len) && log_line[i] ) {
                int         i_end = i + 1;
                
                while ( (i_end < log_line_len) && log_line[i_end] ) {
                    if ( log_line[i_end] == '\n' ) break;
                    i_end++;
                }
                if ( log_line[i_end] == '\n' ) log_line[i_end] = '\0';
                
                if ( __logformat & kI8080LoggingFormatTimestamp ) {
                    struct tm       ts;
                    time_t          now = time(NULL);
                    char            time_str[32];
                    
                    localtime_r(&now, &ts);
                    strftime(time_str, sizeof(time_str), "[%Y-%m-%dT%H:%M:%S] ", &ts);
                    fprintf(logstream, "%s", time_str);
                }
                if ( __logformat & kI8080LoggingFormatPID ) {
                    fprintf(logstream, "(pid=%d) ", getpid());
                }
                if ( __logformat & kI8080LoggingFormatLevelNumber ) {
                    fprintf(logstream, "(lvl=%d) ", level);
                }
                if ( __logformat & kI8080LoggingFormatLevelName ) {
                    fprintf(logstream, "%s: ", I8080LoggingLevelNameForLevel(level));
                }
                fprintf(logstream, "%s\n", &log_line[i]);
                i = i_end + 1;
            }
            if ( (__logformat & kI8080LoggingFormatSourceLoc) && src_file ) {
                fprintf(logstream, "    ↳ %s:%d", src_file, line_no);
                if ( fn_name && (__logformat & kI8080LoggingFormatSourceFunc) )
                    fprintf(logstream, " → %s ", fn_name);
                fputc('\n', logstream);
            } else if ( fn_name && (__logformat & kI8080LoggingFormatSourceFunc) ) {
                fprintf(logstream, "    ↳ %s\n", fn_name);
            }
        } else {
            bool            has_printed = false;

            if ( __logformat & kI8080LoggingFormatTimestamp ) {
                struct tm       ts;
                time_t          now = time(NULL);
                char            time_str[32];
                
                localtime_r(&now, &ts);
                strftime(time_str, sizeof(time_str), "[%Y-%m-%dT%H:%M:%S] ", &ts);
                fprintf(logstream, "%s", time_str);
                has_printed = true;
            }
            if ( __logformat & kI8080LoggingFormatPID ) {
                fprintf(logstream, "(pid=%d) ", getpid());
                has_printed = true;
            }
            if ( __logformat & kI8080LoggingFormatLevelNumber ) {
                fprintf(logstream, "(lvl=%d) ", level);
                has_printed = true;
            }
            if ( __logformat & kI8080LoggingFormatLevelName ) {
                fprintf(logstream, "%s ", I8080LoggingLevelNameForLevel(level));
                has_printed = true;
            }
            if ( (__logformat & kI8080LoggingFormatSourceLoc) && src_file ) {
                if ( has_printed ) fprintf(logstream, "\n   ");
                fprintf(logstream, " ↳ %s:%d", src_file, line_no);
                if ( fn_name && (__logformat & kI8080LoggingFormatSourceFunc) )
                    fprintf(logstream, " → %s", fn_name);
                fputc('}', logstream);
            } else if ( fn_name && (__logformat & kI8080LoggingFormatSourceFunc) ) {
                if ( has_printed ) fprintf(logstream, "\n   ");
                fprintf(logstream, " ↳ %s", fn_name);
            }
            fputc('\n', logstream);
        }
    }
}
