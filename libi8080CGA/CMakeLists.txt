#
# CMake infrastructure for the Curses Graphics Array display
#

#
# Find the ncurses library
#
find_package(Curses REQUIRED)

# Source files for the library:
set(LIBI8080CGA_SOURCES
    I8080CGA.c
    I8080CGAColor.c)
# Header files for the library:
set(LIBI8080CGA_HEADERS
    I8080CGA.h
    I8080CGAColor.h)

add_library(i8080CGA SHARED)
target_sources(i8080CGA
    PRIVATE
        ${LIBI8080CGA_SOURCES}

    PUBLIC
        FILE_SET static_headers
        TYPE HEADERS
        FILES
            ${LIBI8080CGA_HEADERS}
)
target_link_libraries(i8080CGA
    PUBLIC
        i8080
        Curses)


add_executable(cga_demo)
target_sources(cga_demo
    PRIVATE
        cga_demo.c)
target_link_libraries(cga_demo
    PRIVATE
        i8080CGA)
