#!/bin/sh

brew update

#Upgrade packages included on the default image
#See https://docs.travis-ci.com/user/reference/osx/#Compilers-and-Build-toolchain
brew outdated cmake || brew upgrade cmake
brew outdated pkgconfig || brew upgrade pkgconfig

#Install packages not included on the default image
brew install openal-soft
brew install sdl2
