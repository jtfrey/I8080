#
# Demo program that runs an 8080 program that reads
# characters from device 0, converts letters to uppercase,
# and exits when CTRL-D or EOF is encountered.
#
add_executable(uc-echo)
target_sources(uc-echo
    PRIVATE
        uc-echo.c)
target_link_libraries(uc-echo
    PRIVATE
        i8080)
