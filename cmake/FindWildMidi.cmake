# - Find WildMidi
# Find the WildMidi libraries
#
#  This module defines the following variables:
#     WILDMIDI_FOUND        - True if WILDMIDI_INCLUDE_DIR & WILDMIDI_LIBRARY are found
#     WILDMIDI_INCLUDE_DIRS - where to find wildmidi_lib.h, etc.
#     WILDMIDI_LIBRARIES    - the WildMidi library
#

#=============================================================================
# Copyright 2009-2011 Kitware, Inc.
# Copyright 2009-2011 Philip Lowman <philip@yhbt.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#  * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
#  * The names of Kitware, Inc., the Insight Consortium, or the names of
#    any consortium members, or of any contributors, may not be used to
#    endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

find_path(WILDMIDI_INCLUDE_DIR NAMES wildmidi_lib.h
	HINTS $ENV{WILDMIDIDIR}
	DOC "The WildMidi include directory"
)

find_library(WILDMIDI_LIBRARY NAMES WildMidi wildmidi_dynamic libWildMidi
	HINTS $ENV{WILDMIDIDIR}
     DOC "The WildMidi library"
)

# handle the QUIETLY and REQUIRED arguments and set WILDMIDI_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WildMidi REQUIRED_VARS WILDMIDI_LIBRARY WILDMIDI_INCLUDE_DIR)

if(WILDMIDI_FOUND)
    set(WILDMIDI_LIBRARIES ${WILDMIDI_LIBRARY})
    set(WILDMIDI_INCLUDE_DIRS ${WILDMIDI_INCLUDE_DIR})
endif(WILDMIDI_FOUND)

mark_as_advanced(WILDMIDI_INCLUDE_DIR WILDMIDI_LIBRARY)
