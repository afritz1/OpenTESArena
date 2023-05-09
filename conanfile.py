import os
from conan import ConanFile
from conan.tools.files import copy


class OpenTESArena(ConanFile):
    name = "OpenTESArena"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("openal-soft/1.22.2")
        self.requires("sdl/2.26.1")
        self.requires("wildmidi/0.4.5")
