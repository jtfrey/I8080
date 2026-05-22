/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 text buffer API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080TEXTBUFFER_H__
#define __I8080TEXTBUFFER_H__

#include "I8080Config.h"

/**
 * Opaque type of a pointer to a text buffer
 */
typedef struct I8080TextBuffer * I8080TextBufferRef;

/**
 * Create a new text buffer
 * Allocates and initializes a new (empty) text buffer.
 * @return              the new text buffer or NULL if an error
 *                      occurred
 */
I8080TextBufferRef I8080TextBufferCreate(void);

/**
 * Destroy a text buffer
 * Deallocate resources used by \p tbuff and invalidate it.
 * @param tbuff         the text buffer to destroy
 */
void I8080TextBufferDestroy(I8080TextBufferRef tbuff);

/**
 * Length of text currently in a text buffer
 * Returns the number of characters present in \p tbuff.
 * @param tbuff         the text buffer to query
 * @return              the number of characters
 */
size_t I8080TextBufferGetLength(I8080TextBufferRef tbuff);

/**
 * C string pointer associated with a text buffer
 * Returns the C string pointer to the characters in \p tbuff.
 * @param tbuff         the text buffer to query
 * @return              a constant C string pointer -- do NOT
 *                      modify the contents pointed to!
 */
const char* I8080TextBufferGetCStringPtr(I8080TextBufferRef tbuff);

/**
 * Append a sequence of characters to a text buffer
 * Append \p cstr_length characters at \p cstr to the text buffer,
 * \p tbuff.
 * @param tbuff         the text buffer to modify
 * @param cstr          pointer to the characters to append
 * @param cstr_length   if < 0, strlen(cstr) is used to find the
 *                      number of characters to append; if >= 0,
 *                      cstr_length characters will be copied,
 *                      period.
 * @return              the number of characters appended
 */
int I8080TextBufferAppend(I8080TextBufferRef tbuff, const char *cstr, int cstr_length);

/**
 * Append a formatted output string to a text buffer
 * Append characters produced by the printf-style \p fmt string with
 * substitutions made using additional arguments to the function.
 * \p tbuff.
 * @param tbuff         the text buffer to modify
 * @param fmt           a printf-style format string
 * @return              the number of characters appended
 */
int I8080TextBufferPrintf(I8080TextBufferRef tbuff, const char *fmt, ...);

#endif /* __I8080TEXTBUFFER_H__ */
