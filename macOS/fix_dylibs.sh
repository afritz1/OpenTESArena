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
BREW_VULKAN_LIB=/usr/local/lib/libvulkan.1.3.280.dylib
BREW_MOLTENVK_LIB=/usr/local/lib/libMoltenVK.dylib

install_name_tool -change "${BREW_SDL2_LIB}" @executable_path/../Frameworks/libSDL2.dylib ${TES_APP_BUNDLE_PATH}/Contents/MacOS/${BINARY_NAME}
install_name_tool -id @executable_path/../Frameworks/libSDL2.dylib ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libSDL2.dylib
codesign --remove-signature ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libSDL2.dylib

install_name_tool -change "${BREW_OPENAL_SOFT_LIB}" @executable_path/../Frameworks/libopenal.dylib ${TES_APP_BUNDLE_PATH}/Contents/MacOS/${BINARY_NAME}
install_name_tool -id @executable_path/../Frameworks/libopenal.dylib ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libopenal.dylib
codesign --remove-signature ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libopenal.dylib

# @todo if HAVE_WILDMIDI
install_name_tool -change "${BREW_WILDMIDI_LIB}" @executable_path/../Frameworks/libWildMidi.dylib ${TES_APP_BUNDLE_PATH}/Contents/MacOS/${BINARY_NAME}
install_name_tool -id @executable_path/../Frameworks/libWildMidi.dylib ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libWildMidi.dylib
codesign --remove-signature ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libWildMidi.dylib

# @todo if HAVE_VULKAN
install_name_tool -change "${BREW_VULKAN_LIB}" @executable_path/../Frameworks/libvulkan.dylib ${TES_APP_BUNDLE_PATH}/Contents/MacOS/${BINARY_NAME}
install_name_tool -change @rpath/libvulkan.1.dylib @executable_path/../Frameworks/libvulkan.dylib ${TES_APP_BUNDLE_PATH}/Contents/MacOS/${BINARY_NAME}
install_name_tool -add_rpath @loader_path ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libvulkan.dylib
install_name_tool -id @executable_path/../Frameworks/libvulkan.dylib ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libvulkan.dylib
codesign --remove-signature ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libvulkan.dylib

install_name_tool -change "${BREW_MOLTENVK_LIB}" @loader_path/libMoltenVK.dylib ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libvulkan.dylib
install_name_tool -id @loader_path/libMoltenVK.dylib ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libMoltenVK.dylib
codesign --remove-signature ${TES_APP_BUNDLE_PATH}/Contents/Frameworks/libMoltenVK.dylib

codesign -s - --deep -o linker-signed --force "${TES_APP_BUNDLE_PATH}"
