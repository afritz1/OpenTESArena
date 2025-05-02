CMAKE_MINIMUM_REQUIRED(VERSION 3.18.0 FATAL_ERROR)

IF (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW) # Relative linker paths for more portability
ENDIF (COMMAND cmake_policy)

PROJECT(OpenTESArena)

include(FetchContent)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Set global C++ standard for all targets.
set(CMAKE_CXX_STANDARD 20)
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
    # Statically link runtime lib via /MT or /MTd.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Multi-processor compilation.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

    # Disable runtime type info.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")

    # Add various optimizations to release builds.
    SET(CMAKE_CXX_FLAGS_RELEASEGENERICNOLTO "/O2 /Ob2 /Oi /Ot /Oy /fp:fast /Zi")
    SET(CMAKE_CXX_FLAGS_RELEASEGENERIC "${CMAKE_CXX_FLAGS_RELEASEGENERICNOLTO} /GL")
    SET(CMAKE_CXX_FLAGS_RELEASENATIVE "${CMAKE_CXX_FLAGS_RELEASEGENERIC}")
    #SET(CMAKE_CXX_FLAGS_RELEASENATIVE "${CMAKE_CXX_FLAGS_RELEASENATIVE} /arch:AVX512")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERICNOLTO "${CMAKE_EXE_LINKER_FLAGS_RELEASEGENERICNOLTO} /DEBUG")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC "${CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC} /DEBUG /LTCG")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASENATIVE "${CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC}")
ELSEIF (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang") # GCC, {Apple}Clang, MSYS2.
    # To use std::thread and friends, we must pass -lpthread and -pthread to the compiler and Linker for GCC.
    IF (NOT WIN32 AND NOT APPLE)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -pthread")
    ENDIF ()

    # Disable runtime type info (avoids linker error w/ Jolt DebugRendererSimple).
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

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

ADD_COMPILE_DEFINITIONS(JPH_NO_FORCE_INLINE) # Fix __forceinline compile error in LTO builds.
ADD_COMPILE_DEFINITIONS(JPH_DEBUG_RENDERER) # Enable Jolt Physics debug rendering in all builds.

# Include Jolt
FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
    GIT_TAG 0373ec0dd762e4bc2f6acdb08371ee84fa23c6db # 5.3.0
    SOURCE_SUBDIR "Build")
FetchContent_MakeAvailable(JoltPhysics)

ADD_SUBDIRECTORY(components)
ADD_SUBDIRECTORY(OpenTESArena)
