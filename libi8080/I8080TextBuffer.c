/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 text buffer API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */

#include "I8080TextBuffer.h"
#include "I8080Logging.h"

#ifndef I8080_TEXTBUFFER_DELTA_CAPACITY
#   define I8080_TEXTBUFFER_DELTA_CAPACITY 128
#endif

//

typedef struct I8080TextBuffer {
    size_t      capacity;
    size_t      length;
    char        *content;
} I8080TextBuffer_t;

//

I8080TextBufferRef
I8080TextBufferCreate(void)
{
    I8080TextBuffer_t   *new_tbuff = malloc(sizeof(I8080TextBuffer_t));
    
    if ( new_tbuff ) {
        new_tbuff->capacity = new_tbuff->length = 0;
        new_tbuff->content = NULL;
    }
    return (I8080TextBufferRef)new_tbuff;
}

//

void
I8080TextBufferDestroy(
    I8080TextBufferRef  tbuff
)
{
    if ( tbuff->content ) free((void*)tbuff->content);
    free((void*)tbuff);
}

//

size_t
I8080TextBufferGetLength(
    I8080TextBufferRef  tbuff
)
{
    return tbuff->length;
}

//

const char*
I8080TextBufferGetCStringPtr(
    I8080TextBufferRef  tbuff
)
{
    return tbuff->content ? tbuff->content : "";
}

//

static inline
size_t
I8080TextBufferCalcCapacity(
    size_t      target_capacity
)
{
    size_t      chunks = (target_capacity + 127) / 128;
    return chunks * 128;
}

//

int
I8080TextBufferAppend(
    I8080TextBufferRef  tbuff,
    const char          *cstr,
    int                 cstr_length
)
{
    if ( cstr_length < 0 ) cstr_length = strlen(cstr);
    
    if ( cstr_length == 0 ) return 0;
    
    if ( tbuff->length + cstr_length >= tbuff->capacity ) {
        size_t          new_capacity = I8080TextBufferCalcCapacity(tbuff->capacity + cstr_length + 1);
        char            *new_ptr = realloc(tbuff->content, new_capacity);
        
        if ( ! new_ptr ) {
            ERROR("Unable to grow text buffer %p", tbuff);
            return -1;
        }
        tbuff->content = new_ptr;
        tbuff->capacity = new_capacity;
    }
    memcpy(tbuff->content + tbuff->length, cstr, cstr_length);
    tbuff->length += cstr_length;
    *(tbuff->content + tbuff->length) = '\0';
    return cstr_length;
}

//

int
I8080TextBufferPrintf(
    I8080TextBufferRef  tbuff,
    const char          *fmt,
    ...
)
{
    va_list             argv;
    int                 nchar;
    
    va_start(argv, fmt);
    nchar = vsnprintf(NULL, 0, fmt, argv);
    va_end(argv);
    
    if ( nchar == 0 ) return 0;
    
    if ( tbuff->length + nchar >= tbuff->capacity ) {
        size_t          new_capacity = I8080TextBufferCalcCapacity(tbuff->capacity + nchar + 1);
        char            *new_ptr = realloc(tbuff->content, new_capacity);
        
        if ( ! new_ptr ) {
            ERROR("Unable to grow text buffer %p", tbuff);
            return -1;
        }
        tbuff->content = new_ptr;
        tbuff->capacity = new_capacity;
    }
    
    va_start(argv, fmt);
    nchar = vsnprintf(
                tbuff->content + tbuff->length,
                tbuff->capacity - tbuff->length + 1,
                fmt,
                argv);
    va_end(argv);
    tbuff->length += nchar;
    return nchar;
}
