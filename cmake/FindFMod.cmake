# - Try to find FMod
# Once done this will define
#
#  FMOD_FOUND - system has Irrlicht
#  FMOD_INCLUDE_DIRS - the Irrlicht include directory
#  FMOD_LIBRARIES - Link these to use Irrlicht
#
#  Based on FindIrrlicht.cmake
#  Copyright (c) 2008 Olof Naessen <olof.naessen@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if (FMOD_LIBRARIES AND FMOD_INCLUDE_DIRS)
  # in cache already
  set(FMOD_FOUND TRUE)
else (FMOD_LIBRARIES AND FMOD_INCLUDE_DIRS)
  find_path(FMOD_INCLUDE_DIR
    NAMES
      fmod.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
    PATH_SUFFIXES
      fmod
  )

  find_library(FMOD_LIBRARY
    NAMES
      fmod
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
  )

  set(FMOD_INCLUDE_DIRS
    ${FMOD_INCLUDE_DIR}
  )
  set(FMOD_LIBRARIES
    ${FMOD_LIBRARY}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FMod DEFAULT_MSG FMOD_LIBRARY FMOD_INCLUDE_DIRS)

# show the FMOD_INCLUDE_DIRS and FMOD_LIBRARIES variables only in the advanced view
mark_as_advanced(FMOD_INCLUDE_DIRS FMOD_LIBRARIES)

IF (ENET_FOUND)
    MESSAGE(STATUS "Found FMod: " ${FMOD_LIBRARY})
ENDIF (ENET_FOUND)

endif (FMOD_LIBRARIES AND FMOD_INCLUDE_DIRS)
