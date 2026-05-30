/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Baseline header includes and compile-time attributes of the
 * project.
 */

#include "I8080Config.h"

const char     *I8080VersionString = I8080_VERSION_STRING;
const unsigned I8080VersionMajor = I8080_VERSION_MAJOR;
const unsigned I8080VersionMinor = I8080_VERSION_MINOR;
const unsigned I8080VersionPatch = I8080_VERSION_PATCH;

//

off_t
I8080SizeOfNamedFile(
    const char      *filename
)
{
    struct stat     finfo;
    return (stat(filename, &finfo)) ? 0 : finfo.st_size;
}

//

off_t
I8080SizeofFilePtr(
    FILE            *fptr
)
{
    return I8080SizeOfFileDescriptor(fileno(fptr));
}

//

off_t
I8080SizeOfFileDescriptor(
    int             fd
)
{
    struct stat     finfo;
    return (fstat(fd, &finfo)) ? 0 : finfo.st_size;
}
