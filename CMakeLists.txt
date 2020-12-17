CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)

IF (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
ENDIF (COMMAND cmake_policy)

PROJECT(OpenTESArena)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Set global C++ standard for all targets.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Configure for GNU Compiler (also affects MSYS2). Use "MATCHES" instead of "STREQUAL"
# to cover both Clang and AppleClang.
IF (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # To use std::thread and friends, we must pass -lpthread and -pthread to the compiler and Linker for GCC
    IF (NOT WIN32 AND NOT APPLE)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -pthread")
    ENDIF ()

    # Keep debug info.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

    # Optimizations. These settings are meant to work with a variety of CPUs.
    # Modify for your personal build if desired.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffp-contract=fast")

    # Sanitizers (some can affect performance).
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
ENDIF ()

# We want RelWithDebInfo to actually include debug stuff (define _DEBUG
# instead of NDEBUG)
FOREACH(flag_var CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    IF(${flag_var} MATCHES "-DNDEBUG")
        STRING(REGEX REPLACE "-DNDEBUG" "-D_DEBUG" ${flag_var} "${${flag_var}}")
    ENDIF()
ENDFOREACH()

IF (MSVC)
    # Add multi-processor compilation.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
ENDIF ()

ADD_SUBDIRECTORY(components)
ADD_SUBDIRECTORY(OpenTESArena)
