IF (MSVC)
    # DPI-awareness property "VS_DPI_AWARE" is in CMake 3.16.
    # (as of 6/20/2021, Debian is CMake 3.13)
    CMAKE_MINIMUM_REQUIRED(VERSION 3.16.0)
ELSE ()
    CMAKE_MINIMUM_REQUIRED(VERSION 3.8.0)
ENDIF()

IF (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
ENDIF (COMMAND cmake_policy)

PROJECT(OpenTESArena)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Set global C++ standard for all targets.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Replace default configurations (Debug/Release/RelWithDebInfo/MinSizeRel) with custom ones.
# - Debug: maximum debugging
# - ReleaseGenericNoLTO: Same as ReleaseGeneric but for faster dev iteration
# - ReleaseGeneric: distributed at project releases, works with the most CPUs
# - ReleaseNative: maximum performance for the computer it's compiled on
set(CMAKE_CONFIGURATION_TYPES "Debug;ReleaseGenericNoLTO;ReleaseGeneric;ReleaseNative" CACHE STRING "" FORCE)

# CMake-required flags for new configurations.
SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERICNOLTO "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASENATIVE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

# Compiler settings (optimizations, debugging, etc.)
IF (MSVC)
    # Multi-processor compilation.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

    # Add various optimizations to release builds.
    SET(CMAKE_CXX_FLAGS_RELEASEGENERICNOLTO "/O2 /Ob2 /Oi /Ot /Oy /fp:fast")
    SET(CMAKE_CXX_FLAGS_RELEASEGENERIC "${CMAKE_CXX_FLAGS_RELEASEGENERICNOLTO} /GL")
    SET(CMAKE_CXX_FLAGS_RELEASENATIVE "${CMAKE_CXX_FLAGS_RELEASEGENERIC}")
    #SET(CMAKE_CXX_FLAGS_RELEASENATIVE "${CMAKE_CXX_FLAGS_RELEASENATIVE} /arch:AVX2")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC "${CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC} /LTCG")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASENATIVE "${CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC}")
ELSEIF (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang") # GCC, {Apple}Clang, MSYS2.
    # To use std::thread and friends, we must pass -lpthread and -pthread to the compiler and Linker for GCC.
    IF (NOT WIN32 AND NOT APPLE)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -pthread")
    ENDIF ()

    # Debug builds.
    SET(CMAKE_CXX_FLAGS_DEBUG "-g")

    # Generic release build (works with a variety of CPUs).
    SET(CMAKE_CXX_FLAGS_RELEASEGENERICNOLTO "-g -O3")
    SET(CMAKE_CXX_FLAGS_RELEASEGENERIC "${CMAKE_CXX_FLAGS_RELEASEGENERICNOLTO} -flto")

    # Native release build for the computer compiling it.
    SET(CMAKE_CXX_FLAGS_RELEASENATIVE "-g -Ofast -flto -march=native")
    #SET(CMAKE_CXX_FLAGS_RELEASENATIVE "${CMAKE_CXX_FLAGS_RELEASENATIVE} -mavx")
    #SET(CMAKE_CXX_FLAGS_RELEASENATIVE "${CMAKE_CXX_FLAGS_RELEASENATIVE} -mfpu=neon")

    # Sanitizers (can affect performance).
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
ENDIF ()

ADD_SUBDIRECTORY(components)
ADD_SUBDIRECTORY(OpenTESArena)
