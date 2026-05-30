#
# CLI 8080 emulator
#

set(I8E8E_CGA_SOFT_PAGELIMIT "0x20" CACHE STRING "Warn for CGA resolutions > this many pages")
set(I8E8E_CGA_HARD_PAGELIMIT "0x40" CACHE STRING "Error for CGA resolutions > this many pages")

if ( I8E8E_CGA_SOFT_PAGELIMIT GREATER I8E8E_CGA_HARD_PAGELIMIT )
    message(FATAL_ERROR "The I8E8E_CGA_SOFT_PAGELIMIT exceeds the I8E8E_CGA_HARD_PAGELIMIT.")
endif()

add_executable(i8e8e)
target_sources(i8e8e
    PRIVATE
        I8e8e.c
        I8e8eMemObj.c
        I8e8eBSMObj.c
        I8e8eFileDeviceObj.c

    PUBLIC
        FILE_SET static_headers
        TYPE HEADERS
        FILES
            I8e8eMemObj.h
            I8e8eBSMObj.h
            I8e8eFileDeviceObj.h)
target_compile_definitions(i8e8e
    PRIVATE
        I8E8E_CGA_SOFT_PAGELIMIT=${I8E8E_CGA_SOFT_PAGELIMIT}
        I8E8E_CGA_HARD_PAGELIMIT=${I8E8E_CGA_HARD_PAGELIMIT})
target_link_libraries(i8e8e
    PRIVATE
        i8080
        i8080CGA
        i8080Timer
        i8080BSM
        i8080PPU)
