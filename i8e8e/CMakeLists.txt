#
# CLI 8080 emulator
#

set(I8E8E_CGA_SOFT_PAGELIMIT "0x20" CACHE STRING "Warn for CGA resolutions > this many pages")
set(I8E8E_CGA_HARD_PAGELIMIT "0x40" CACHE STRING "Error for CGA resolutions > this many pages")

if ( I8E8E_CGA_SOFT_PAGELIMIT GREATER I8E8E_CGA_HARD_PAGELIMIT )
    message(FATAL_ERROR "The I8E8E_CGA_SOFT_PAGELIMIT exceeds the I8E8E_CGA_HARD_PAGELIMIT.")
endif()

option(I8E8E_ENABLE_PPU "Include the I8080 PPU in the emulator" ON)
option(I8E8E_ENABLE_APU "Include the I8080 APU in the emulator" I8080_OS_MACOSX)

set(i8e8e_SOURCES
        I8e8e.c
        I8e8eMemObj.c
        I8e8eBSMObj.c
        I8e8eFileDeviceObj.c)

set(i8e8e_HEADERS
            I8e8eMemObj.h
            I8e8eBSMObj.h
            I8e8eFileDeviceObj.h)

set(i8e8e_LIBRARIES
        i8080
        i8080CGA
        i8080Timer
        i8080BSM)
        
add_executable(i8e8e)

if ( I8E8E_ENABLE_PPU )
    list(APPEND i8e8e_LIBRARIES i8080PPU)
    target_compile_definitions(i8e8e
        PRIVATE
            I8E8E_HAVE_PPU)
endif ()

if ( I8E8E_ENABLE_APU )
    list(APPEND i8e8e_LIBRARIES i8080APU)
    target_compile_definitions(i8e8e
        PRIVATE
            I8E8E_HAVE_APU)
endif ()

target_sources(i8e8e
    PRIVATE
        ${i8e8e_SOURCES}
    PUBLIC
        FILE_SET static_headers
        TYPE HEADERS
        FILES
            ${i8e8e_HEADERS})
            
target_compile_definitions(i8e8e
    PRIVATE
        I8E8E_CGA_SOFT_PAGELIMIT=${I8E8E_CGA_SOFT_PAGELIMIT}
        I8E8E_CGA_HARD_PAGELIMIT=${I8E8E_CGA_HARD_PAGELIMIT})
target_link_libraries(i8e8e
    PRIVATE
        ${i8e8e_LIBRARIES})
