#!/bin/bash

BINARY_NAME=otesa

# Attempted many, many, MANY, things with realpath, readlink, brew, etc. but couldn't programmatically
# resolve the exact dylib paths shown by otool -L, so hardcoding for now :/ very upset
#BREW=/opt/homebrew/bin/brew
#BREW_SDL2_LIB=$(BREW --prefix sdl2)/lib/libSDL2-2.0.0.dylib
BREW_SDL2_LIB=/opt/homebrew/opt/sdl2/lib/libSDL2-2.0.0.dylib
#BREW_OPENAL_SOFT_LIB=$(BREW --prefix openal-soft)/lib/libopenal.1.24.3.dylib
BREW_OPENAL_SOFT_LIB=/System/Library/Frameworks/OpenAL.framework/Versions/A/OpenAL
#BREW_WILDMIDI_LIB=$(BREW --prefix wildmidi)/lib/libWildMidi.2.dylib
BREW_WILDMIDI_LIB=/opt/homebrew/opt/wildmidi/lib/libWildMidi.2.dylib

install_name_tool -change "${BREW_SDL2_LIB}" @executable_path/../Frameworks/libSDL2.dylib ${TES_APP_BUNDLE_PATH}/Contents/MacOS/${BINARY_NAME}
install_name_tool -id @executable_path/../Frameworks/libSDL2.dylib ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libSDL2.dylib

install_name_tool -change "${BREW_OPENAL_SOFT_LIB}" @executable_path/../Frameworks/libopenal.dylib ${TES_APP_BUNDLE_PATH}/Contents/MacOS/${BINARY_NAME}
install_name_tool -id @executable_path/../Frameworks/libopenal.dylib ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libopenal.dylib

# @todo if HAVE_WILDMIDI
install_name_tool -change "${BREW_WILDMIDI_LIB}" @executable_path/../Frameworks/libWildMidi.dylib ${TES_APP_BUNDLE_PATH}/Contents/MacOS/${BINARY_NAME}
install_name_tool -id @executable_path/../Frameworks/libWildMidi.dylib ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libWildMidi.dylib
