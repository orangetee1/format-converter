cmake_minimum_required(VERSION 3.25)

project(lab4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_property(LAB4_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT LAB4_IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(LAB4_BUILD_TESTS "Build unit tests (GoogleTest)" ON)
option(LAB4_WERROR "Treat compiler warnings as errors" OFF) 

add_library(lab4_warnings INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lab4_warnings INTERFACE
        -Wall        
        -Wextra      
        -Wpedantic   
        -Wconversion 
        -Wshadow   
    )
    if(LAB4_WERROR)
        target_compile_options(lab4_warnings INTERFACE -Werror)
    endif()
endif()

add_subdirectory(src)

if(LAB4_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
