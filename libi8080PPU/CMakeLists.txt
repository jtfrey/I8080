#
# CMake infrastructure for the PPU adapter
#

if(I8080_OS_MACOSX)
    option(I8080_PPU_OSX_KEYCHECKS "Use Mac OS X key events in lieu of curses" TRUE)
endif()

# Source files for the library:
set(LIBI8080PPU_SOURCES
    I8080PPU.c
    I8080DPad.c)
# Header files for the library:
set(LIBI8080PPU_HEADERS
    I8080PPU.h
    I8080DPad.h)

add_library(i8080PPU SHARED)
target_sources(i8080PPU
    PRIVATE
        ${LIBI8080PPU_SOURCES}

    PUBLIC
        FILE_SET static_headers
        TYPE HEADERS
        FILES
            ${LIBI8080PPU_HEADERS}
)
target_link_libraries(i8080PPU
    PUBLIC
        i8080
        i8080CGA)
if(I8080_PPU_OSX_KEYCHECKS)
    target_compile_definitions(i8080PPU PRIVATE I8080_PPU_OSX_KEYCHECKS)
    target_link_libraries(i8080PPU
        PRIVATE
            "-framework Carbon")
endif()

add_executable(ppu_demo)
target_sources(ppu_demo
    PRIVATE
        ppu_demo.c)
target_link_libraries(ppu_demo
    PRIVATE
        i8080PPU)
