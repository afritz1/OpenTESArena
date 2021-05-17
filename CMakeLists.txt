CMAKE_MINIMUM_REQUIRED(VERSION 3.8.0)

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
# - DebugFast: like Debug but for typical development
# - ReleaseGeneric: distributed at project releases, works with most CPUs
# - ReleaseNative: maximum performance for the computer it's compiled on
set(CMAKE_CONFIGURATION_TYPES "Debug;DebugFast;ReleaseGeneric;ReleaseNative" CACHE STRING "" FORCE)

# CMake-required flags for new configurations.
SET(CMAKE_EXE_LINKER_FLAGS_DEBUGFAST "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASENATIVE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

# Compiler settings (optimizations, debugging, etc.)
IF (MSVC)
    # Multi-processor compilation.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

    # Add custom debug flags to DebugFast (/RTCs and /RTCu are not compatible with /Ox).
    SET(CMAKE_CXX_FLAGS_DEBUGFAST "/DEBUG /Zi /MDd /Ox /Ob0")
    SET(CMAKE_RC_FLAGS_DEBUGFAST "${CMAKE_RC_FLAGS_DEBUG}")

    # Add various optimizations to release builds.
    SET(CMAKE_CXX_FLAGS_RELEASEGENERIC "/O2 /Ob2 /Oi /Ot /Oy /GL /fp:fast")
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
    SET(CMAKE_CXX_FLAGS_DEBUGFAST "-g -Og")

    # Generic release build (works with a variety of CPUs).
    SET(CMAKE_CXX_FLAGS_RELEASEGENERIC "-g -O3 -flto")

    # Native release build for the computer compiling it.
    SET(CMAKE_CXX_FLAGS_RELEASENATIVE "-g -Ofast -flto -march=native")
    #SET(CMAKE_CXX_FLAGS_RELEASENATIVE "${CMAKE_CXX_FLAGS_RELEASENATIVE} -mavx")
    #SET(CMAKE_CXX_FLAGS_RELEASENATIVE "${CMAKE_CXX_FLAGS_RELEASENATIVE} -mfpu=neon")

    # Sanitizers (can affect performance).
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
ENDIF ()

ADD_SUBDIRECTORY(components)
ADD_SUBDIRECTORY(OpenTESArena)
