#
# CMake infrastructure for bank-switch memory
#

# Source files for the library:
set(LIBI8080BSM_SOURCES
    I8080BSM.c)
# Header files for the library:
set(LIBI8080BSM_HEADERS
    I8080BSM.h)

add_library(i8080BSM SHARED)
target_sources(i8080BSM
    PRIVATE
        ${LIBI8080BSM_SOURCES}

    PUBLIC
        FILE_SET static_headers
        TYPE HEADERS
        FILES
            ${LIBI8080BSM_HEADERS}
)
target_link_libraries(i8080BSM
    PUBLIC
        i8080)
