#
# The base CMake build system configuration file for the project.
#
cmake_minimum_required(VERSION 3.10)
project(I8080
    VERSION
        0.0.1
    LANGUAGES
        C ASM)

#
# Which OS?
#
if(APPLE)
    set(I8080_OS_MACOSX true)
elseif(UNIX)
    set(I8080_OS_LINUX true)
else()
    message(FATAL "Unsupported operating system.")
endif()

add_subdirectory(libi8080)

option(I8080CGA_ENABLE "Enable build of the Curses Graphics Array library" ON)
if ( ${I8080CGA_ENABLE} )
add_subdirectory(libi8080CGA)
add_subdirectory(libi8080PPU)
endif ()

option(I8080TIMER_ENABLE "Enable build of the realtime timers library" ON)
if ( ${I8080TIMER_ENABLE} )
add_subdirectory(libi8080Timer)
endif ()

option(I8080BSM_ENABLE "Enable build of the bank-switched memory expander library" ON)
if ( ${I8080BSM_ENABLE} )
add_subdirectory(libi8080BSM)
endif ()

add_subdirectory(demo)
add_subdirectory(i8e8e)