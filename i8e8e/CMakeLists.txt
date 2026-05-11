#
# CLI 8080 emulator
#
add_executable(i8e8e)
target_sources(i8e8e
    PRIVATE
        I8e8e.c
        I8e8eMemObj.c
        I8e8eFileDeviceObj.c

    PUBLIC
        FILE_SET static_headers
        TYPE HEADERS
        FILES
            I8e8eMemObj.h
            I8e8eFileDeviceObj.h)
target_link_libraries(i8e8e
    PRIVATE
        i8080)
