CMAKE_MINIMUM_REQUIRED(VERSION 3.21.0 FATAL_ERROR)

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

# Compiler settings (optimizations, debugging, etc.)
# Due to how Jolt configures solution-wide flags, USE_AVX512 must be ON to get AVX-512.
IF (MSVC)
    # Replace default configurations.
    # - Debug: maximum debugging
    # - ReleaseGenericNoLTO: same as ReleaseGeneric but for faster dev iteration
    # - ReleaseGeneric: optimized for project releases, works with the most CPUs
    set(CMAKE_CONFIGURATION_TYPES "Debug;ReleaseGenericNoLTO;ReleaseGeneric" CACHE STRING "" FORCE)

    # CMake-required flags for new configurations.
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERICNOLTO "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

    # Statically link runtime lib via /MT or /MTd.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Multi-processor compilation.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

    # Disable runtime type info.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")

    # Add various optimizations to release builds.
    SET(CMAKE_CXX_FLAGS_RELEASEGENERICNOLTO "/O2 /Ob2 /Oi /Ot /Oy /fp:fast /Zi")
    SET(CMAKE_CXX_FLAGS_RELEASEGENERIC "${CMAKE_CXX_FLAGS_RELEASEGENERICNOLTO} /GL")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERICNOLTO "${CMAKE_EXE_LINKER_FLAGS_RELEASEGENERICNOLTO} /DEBUG")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC "${CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC} /DEBUG /LTCG")
ELSEIF (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang") # GCC, {Apple}Clang, MSYS2.
    # Replace default configurations.
    # - Debug: maximum debugging
    # - ReleaseGeneric: optimized for project releases, works with the most CPUs
    # - ReleaseNativeNoLTO: faster dev iteration than ReleaseGeneric
    # - ReleaseNative: maximum performance only for the computer compiling it
    set(CMAKE_CONFIGURATION_TYPES "Debug;ReleaseGeneric;ReleaseNativeNoLTO;ReleaseNative" CACHE STRING "" FORCE)

    # CMake-required flags for new configurations.
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASEGENERIC "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASENATIVENOLTO "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASENATIVE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

    # To use std::thread and friends, we must pass -lpthread and -pthread to the compiler and Linker for GCC.
    IF (NOT WIN32 AND NOT APPLE)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -pthread")
    ENDIF ()

    # Disable runtime type info (avoids linker error w/ Jolt DebugRendererSimple).
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

    # Debug build.
    SET(CMAKE_CXX_FLAGS_DEBUG "-g")

    # Optimized release build, works with CPUs of the same arch (e.g. x86-64).
    SET(CMAKE_CXX_FLAGS_RELEASEGENERIC "-g -O3 -flto=auto")

    # Optimized development build with fast compilation.
    # Jolt Physics is sensitive to -ffast-math so be selective instead.
    SET(CMAKE_CXX_FLAGS_RELEASENATIVENOLTO "-g -O3 -march=native")
    SET(CMAKE_CXX_FLAGS_RELEASENATIVENOLTO "${CMAKE_CXX_FLAGS_RELEASENATIVENOLTO} -fno-math-errno")
    SET(CMAKE_CXX_FLAGS_RELEASENATIVENOLTO "${CMAKE_CXX_FLAGS_RELEASENATIVENOLTO} -fno-trapping-math")
    SET(CMAKE_CXX_FLAGS_RELEASENATIVENOLTO "${CMAKE_CXX_FLAGS_RELEASENATIVENOLTO} -fno-signed-zeros")
    SET(CMAKE_CXX_FLAGS_RELEASENATIVENOLTO "${CMAKE_CXX_FLAGS_RELEASENATIVENOLTO} -ffp-contract=on")
    #SET(CMAKE_CXX_FLAGS_RELEASENATIVENOLTO "${CMAKE_CXX_FLAGS_RELEASENATIVENOLTO} -ffinite-math-only")
    #SET(CMAKE_CXX_FLAGS_RELEASENATIVENOLTO "${CMAKE_CXX_FLAGS_RELEASENATIVENOLTO} -freciprocal-math")
    #SET(CMAKE_CXX_FLAGS_RELEASENATIVENOLTO "${CMAKE_CXX_FLAGS_RELEASENATIVENOLTO} -fassociative-math")

    # Max performance build only for the computer compiling it.
    SET(CMAKE_CXX_FLAGS_RELEASENATIVE "${CMAKE_CXX_FLAGS_RELEASENATIVENOLTO} -flto=auto")

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
