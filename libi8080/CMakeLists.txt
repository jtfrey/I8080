#
# CMake infrastructure for the library
#

option(I8080_STRIP_DEBUGGING "Enable this option to eliminate DEBUG statements entirely" OFF)

option(I8080_LOGGING_USE_PRETTY_FUNCTION "Use the __PRETTY_FUNCTION__ macro if on, __func__ otherwise" ON)

option(I8080_ENABLE_MMAP "Enable mmap() ROM mapping in I8080Memory" ON)

option(I8080_MEMORY_ENABLE_INLINING "Enable inlining of I8080Mem read/write functions" OFF)

option(I8080_SYSTEM_ENABLE_INTERRUPT_API "Enable the I8080System interrupt API" ON)

if ( I8080_SYSTEM_ENABLE_INTERRUPT_API ) 
    find_package(Threads REQUIRED)
endif()

#
# Compiled feature checks
#
include(CheckCSourceCompiles)

# Check for __builtin_parity
check_c_source_compiles("
int main() {
    unsigned int i = 42;
    return __builtin_parity(i);
}
" HAS_BUILTIN_PARITY)
if(HAS_BUILTIN_PARITY)
    message(STATUS "Found __builtin_parity")
endif()

# Check for OLCUC output flag in termios
check_c_source_compiles("
#include <termios.h>
int main() {
    return OLCUC;
}
" HAS_TERMIOS_OLCUC)
if(HAS_TERMIOS_OLCUC)
    message(STATUS "Found termios OLCUC")
endif()

if(I8080_ENABLE_MMAP)
    # Check for mmap output flag in termios
    check_c_source_compiles("
#include <sys/mman.h>
int main() {
    void    *p = mmap((void*)0, 0, 0, 0, 0, 0);
    
    munmap(p, 0);
    return 0;
}
    " HAS_MMAP)
    if(HAS_MMAP)
        message(STATUS "Found mmap/munmap")
    endif()
else()
    unset(HAS_MMAP)
    unset(HAS_MMAP CACHE)
endif()    

#
# Generate our common header file and have all targets in this
# directory use the binary directory on the search path:
#
configure_file(I8080Config.h.in I8080Config.h)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Source files for the library:
set(LIBI8080_SOURCES
    I8080Config.c
    I8080Logging.c
    I8080TextBuffer.c
    I8080HostMemory.c
    I8080Registers.c
    I8080Memory.c
    I8080DeviceIO.c
    I8080Instructions.c
    I8080InstrHandlerMisc.c
    I8080InstrHandlerIO.c
    I8080InstrHandlerFlags.c
    I8080InstrHandlerSingleReg.c
    I8080InstrHandlerDataMovement.c
    I8080InstrHandlerAccum.c
    I8080InstrHandlerRegPairs.c
    I8080InstrHandlerDirectAddr.c
    I8080InstrHandlerJump.c
    I8080Disassembler.c
    I8080System.c)
# Header files for the library:
set(LIBI8080_HEADERS
    I8080Logging.h
    I8080TextBuffer.h
    I8080HostMemory.h
    I8080Registers.h
    I8080Memory.h
    I8080DeviceIO.h
    I8080Instructions.h
    I8080InstrHandlerMisc.h
    I8080InstrHandlerFlags.h
    I8080InstrHandlerIO.h
    I8080InstrHandlerSingleReg.h
    I8080InstrHandlerDataMovement.h
    I8080InstrHandlerAccum.h
    I8080InstrHandlerRegPairs.h
    I8080InstrHandlerDirectAddr.h
    I8080InstrHandlerJump.h
    I8080Disassembler.h
    I8080System.h
    I8080Timing.h)

add_library(i8080 SHARED)
target_sources(i8080
    PRIVATE
        ${LIBI8080_SOURCES}

    PUBLIC
        FILE_SET static_headers
        TYPE HEADERS
        FILES
            ${LIBI8080_HEADERS}

    PUBLIC
        FILE_SET generated_headers
        TYPE HEADERS
        BASE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/I8080Config.h
)
if ( I8080_SYSTEM_ENABLE_INTERRUPT_API )
    target_link_libraries(i8080
        PUBLIC
            Threads::Threads)
endif()
