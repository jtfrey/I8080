#
# The base CMake build system configuration file for the project.
#
cmake_minimum_required(VERSION 3.10)
project(I8080
    VERSION
        0.0.1
    LANGUAGES
        C ASM)

add_subdirectory(libi8080)
add_subdirectory(demo)
