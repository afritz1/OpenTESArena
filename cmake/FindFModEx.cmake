# - Try to find FMod Ex
# Once done this will define
#
#  FMODEX_FOUND - system has FMod Ex
#  FMODEX_INCLUDE_DIRS - the FMod Ex include directory
#  FMODEX_LIBRARIES - Link these to use FMod Ex
#
#  Based on FindIrrlicht.cmake
#  Copyright (c) 2008 Olof Naessen <olof.naessen@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if (FMODEX_LIBRARIES AND FMODEX_INCLUDE_DIRS)
  # in cache already
  set(FMODEX_FOUND TRUE)

else (FMODEX_LIBRARIES AND FMODEX_INCLUDE_DIRS)
  find_path(FMODEX_INCLUDE_DIR
    NAMES
      fmod.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      HINTS $ENV{FMODDIR}/inc
      HINTS $ENV{FMODDIR}/api/inc
    PATH_SUFFIXES
      fmod
  )

  find_library(FMODEX_LIBRARY
    NAMES
      fmodex
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      HINTS $ENV{FMODDIR}/lib
      HINTS $ENV{FMODDIR}/api/lib
  )

  set(FMODEX_INCLUDE_DIRS
    ${FMODEX_INCLUDE_DIR}
  )
  set(FMODEX_LIBRARIES
    ${FMODEX_LIBRARY}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FModEx DEFAULT_MSG FMODEX_LIBRARY FMODEX_INCLUDE_DIRS)

# show the FMODEX_INCLUDE_DIRS and FMODEX_LIBRARIES variables only in the advanced view
mark_as_advanced(FMODEX_INCLUDE_DIRS FMODEX_LIBRARIES)

IF (FMODEX_LIBRARIES AND FMODEX_INCLUDE_DIRS)
    MESSAGE(STATUS "Found FMod Ex: " ${FMODEX_LIBRARY})
ENDIF (FMODEX_LIBRARIES AND FMODEX_INCLUDE_DIRS)

# Set the FMODDIR environment variable
GET_FILENAME_COMPONENT(FMOD_BASE ${FMODEX_INCLUDE_DIRS} DIRECTORY)
GET_FILENAME_COMPONENT(FMOD_BASE ${FMOD_BASE} DIRECTORY)
SET(ENV{FMODDIR} ${FMOD_BASE})

endif (FMODEX_LIBRARIES AND FMODEX_INCLUDE_DIRS)
