#
# CMake infrastructure for the PPU adapter
#

# Source files for the library:
set(LIBI8080PPU_SOURCES
    I8080PPU.c)
# Header files for the library:
set(LIBI8080PPU_HEADERS
    I8080PPU.h)

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


add_executable(ppu_demo)
target_sources(ppu_demo
    PRIVATE
        ppu_demo.c)
target_link_libraries(ppu_demo
    PRIVATE
        i8080PPU)
