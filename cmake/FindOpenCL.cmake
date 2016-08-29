#.rst:
# FindOpenCL
# ----------
#
# Try to find OpenCL
#
# Once done this will define::
#
#   OpenCL_FOUND          - True if OpenCL was found
#   OpenCL_INCLUDE_DIRS   - include directories for OpenCL
#   OpenCL_LIBRARIES      - link against this library to use OpenCL
#   OpenCL_VERSION_STRING - Highest supported OpenCL version (eg. 1.2)
#   OpenCL_VERSION_MAJOR  - The major version of the OpenCL implementation
#   OpenCL_VERSION_MINOR  - The minor version of the OpenCL implementation
#
# The module will also define two cache variables::
#
#   OpenCL_INCLUDE_DIR    - the OpenCL include directory
#   OpenCL_LIBRARY        - the path to the OpenCL library
#

#=============================================================================
# Copyright 2014 Matthaeus G. Chajdas
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

function(_FIND_OPENCL_VERSION)
  include(CheckSymbolExists)
  include(CMakePushCheckState)
  set(CMAKE_REQUIRED_QUIET ${OpenCL_FIND_QUIETLY})

  CMAKE_PUSH_CHECK_STATE()
  foreach(VERSION "1_2" "1_1" "1_0")
    set(CMAKE_REQUIRED_INCLUDES "${OpenCL_INCLUDE_DIR}")

    if(APPLE)
      # prefer the header from the Framework
      set(OSX_OpenCL_HEADER "${OpenCL_INCLUDE_DIR}/Headers/cl.h")
      if(EXISTS "${OpenCL_INCLUDE_DIR}/OpenCL/cl.h")
        set(OSX_OpenCL_HEADER "${OpenCL_INCLUDE_DIR}/OpenCL/cl.h")
      endif()
    
      CHECK_SYMBOL_EXISTS(
        CL_VERSION_${VERSION}
        ${OSX_OpenCL_HEADER}
        OPENCL_VERSION_${VERSION})
    else()
      CHECK_SYMBOL_EXISTS(
        CL_VERSION_${VERSION}
        "${OpenCL_INCLUDE_DIR}/CL/cl.h"
        OPENCL_VERSION_${VERSION})
    endif()

    if(OPENCL_VERSION_${VERSION})
      string(REPLACE "_" "." VERSION "${VERSION}")
      set(OpenCL_VERSION_STRING ${VERSION} PARENT_SCOPE)
      string(REGEX MATCHALL "[0-9]+" version_components "${VERSION}")
      list(GET version_components 0 major_version)
      list(GET version_components 1 minor_version)
      set(OpenCL_VERSION_MAJOR ${major_version} PARENT_SCOPE)
      set(OpenCL_VERSION_MINOR ${minor_version} PARENT_SCOPE)
      break()
    endif()
  endforeach()
  CMAKE_POP_CHECK_STATE()
endfunction()

find_path(OpenCL_INCLUDE_DIR
  NAMES
    CL/cl.h OpenCL/cl.h
  PATHS
    ENV "PROGRAMFILES(X86)"
    ENV AMDAPPSDKROOT
    ENV INTELOCLSDKROOT
    ENV NVSDKCOMPUTE_ROOT
    ENV CUDA_PATH
    ENV ATISTREAMSDKROOT
  PATH_SUFFIXES
    include
    OpenCL/common/inc
    "AMD APP/include")
    
_FIND_OPENCL_VERSION()

# Once we have a working include directory, try looking specifically for cl2.hpp
find_path(OpenCL_CL2_INCLUDE_DIR
  NAMES
    CL/cl2.hpp OpenCL/cl2.hpp
  PATHS
    ENV "PROGRAMFILES(X86)"
    ENV AMDAPPSDKROOT
    ENV INTELOCLSDKROOT
    ENV NVSDKCOMPUTE_ROOT
    ENV CUDA_PATH
    ENV ATISTREAMSDKROOT
  PATH_SUFFIXES
    include
    OpenCL/common/inc
    "AMD APP/include")
    
# We add a library suffix because some OpenCL packages won't provide a softlink without any, like: http://packages.ubuntu.com/trusty/amd64/nvidia-libopencl1-331/filelist
# So hopefully .so.1 matches those cases
list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES .so.1)

# We just search the directory prefixes regardless of architecture; really only the proper one should ever appear anyway and otherwise it can easily be overridden in cmake-gui
find_library(OpenCL_LIBRARY
  NAMES OpenCL
  PATHS
    ENV "PROGRAMFILES(X86)"
    ENV "PROGRAMFILES(X86)"
    ENV AMDAPPSDKROOT
    ENV INTELOCLSDKROOT
    ENV CUDA_PATH
    ENV NVSDKCOMPUTE_ROOT
    ENV ATISTREAMSDKROOT
  PATH_SUFFIXES
    "AMD APP/lib/x86"
    lib/x86
    lib/Win32

    /usr/lib/x86_64-linux-gnu
    /usr/lib/i386-linux-gnu
    OpenCL/common/lib/Win32)

set(OpenCL_LIBRARIES ${OpenCL_LIBRARY})
set(OpenCL_INCLUDE_DIRS ${OpenCL_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# Ubuntu 12.04 / Travis CI have an old version of CMake that doesn't
# support "FOUND_VAR OpenCL_FOUND". This could, in principle, be added
# at a later date.
find_package_handle_standard_args(
  OpenCL
  REQUIRED_VARS OpenCL_LIBRARY OpenCL_INCLUDE_DIR OpenCL_CL2_INCLUDE_DIR
  VERSION_VAR OpenCL_VERSION_STRING)

mark_as_advanced(
  OpenCL_INCLUDE_DIR
  OpenCL_LIBRARY)
