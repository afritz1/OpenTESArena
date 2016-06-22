CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

IF (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
ENDIF (COMMAND cmake_policy)

PROJECT(OpenTESArena)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Configure for GNU Compiler
IF (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # To use std::thread and friends, we must pass -lpthread and -pthread to the compiler and Linker for GCC
    IF (NOT WIN32)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -pthread")
    ENDIF (NOT WIN32)
ENDIF (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

# We want RelWithDebInfo to actually include debug stuff (define _DEBUG
# instead of NDEBUG)
FOREACH(flag_var  CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    IF(${flag_var} MATCHES "-DNDEBUG")
        STRING(REGEX REPLACE "-DNDEBUG" "-D_DEBUG" ${flag_var} "${${flag_var}}")
    ENDIF()
ENDFOREACH()

ADD_SUBDIRECTORY(components)
ADD_SUBDIRECTORY(OpenTESArena)
