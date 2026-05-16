#
# CMake infrastructure for the realtime timer
#

find_package(Threads REQUIRED)

# Source files for the library:
set(LIBI8080TIMER_SOURCES
    I8080Timer.c)
# Header files for the library:
set(LIBI8080TIMER_HEADERS
    I8080Timer.h)

add_library(i8080Timer SHARED)
target_sources(i8080Timer
    PRIVATE
        ${LIBI8080TIMER_SOURCES}

    PUBLIC
        FILE_SET static_headers
        TYPE HEADERS
        FILES
            ${LIBI8080TIMER_HEADERS}
)
target_link_libraries(i8080Timer
    PUBLIC
        i8080
        Threads::Threads)


add_executable(timer_test)
target_sources(timer_test
    PRIVATE
        timer_test.c)
target_link_libraries(timer_test
    PRIVATE
        i8080Timer)
