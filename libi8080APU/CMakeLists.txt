#
# CMake infrastructure for the APU adapter
#

if(I8080_OS_MACOSX)
    # Source files for the library:
    set(LIBI8080APU_SOURCES
        I8080APU.c
        I8080APUConstants.c)
    # Header files for the library:
    set(LIBI8080APU_HEADERS
        I8080APU.h
        I8080APUConstants.h)
    
    add_library(i8080APU SHARED)
    target_sources(i8080APU
        PRIVATE
            ${LIBI8080APU_SOURCES}
    
        PUBLIC
            FILE_SET static_headers
            TYPE HEADERS
            FILES
                ${LIBI8080APU_HEADERS}
    )
    target_link_libraries(i8080APU
        PUBLIC
            i8080
        PRIVATE
            "-framework AudioToolbox")

    add_executable(apu_demo)
    target_sources(apu_demo
        PRIVATE
            apu_demo.c)
    target_link_libraries(apu_demo
        PRIVATE
            i8080APU)
else()
    message(WARNING "the APU library is for Mac OS X only")
endif()
