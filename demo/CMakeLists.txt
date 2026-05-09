#
# Demo program that runs an 8080 program that reads
# characters from device 0, converts letters to uppercase,
# and exits when CTRL-D or EOF is encountered.
#
add_executable(demo)
target_sources(demo
    PRIVATE
        demo.c)
target_link_libraries(demo
    PRIVATE
        i8080)
