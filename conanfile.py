from conans import ConanFile, CMake
from conans.tools import download, unzip
import os

class GBCEmulator(ConanFile):

    name = "GBCEmulator"
    version = "0.0.1"
    url = "https://github.com/joshgamer474/GBCEmulator"
    description = "A WIP Gameboy (Color) emulator written in C++"
    settings = {"os" : ["Windows"], 
                "arch": ["x86", "x86_64"],
                "compiler": ["Visual Studio"]}
    options = {"shared": [True, False]}
    default_options = "shared=True"
    generators = "cmake"
    requires = "SDL/2.0.8@josh/stable", "spdlog/0.16.3@bincrafters/stable"
    exports_sources = "src/*", "CMakeLists.txt"
    
    def imports(self):
        dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        self.copy("*.dll", src="bin", dst=dest)
        self.keep_imports = True
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        
    def package(self):
        self.copy("*", src="bin", dst="bin",     keep_path=False)
    