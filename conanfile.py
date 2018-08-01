from conans import ConanFile, CMake, tools
import os

class GBCEmulator(ConanFile):

    name = "GBCEmulator"
    version = "0.0.2"
    url = "https://github.com/joshgamer474/GBCEmulator"
    description = "A WIP Gameboy (Color) emulator written in C++"
    settings = {"os" : ["Windows"], 
                "arch": ["x86", "x86_64"],
                "compiler": ["Visual Studio"]}
    options = {"shared": [True, False]}
    generators = "cmake"
    requires = "sdl2/2.0.8@bincrafters/stable", \
                "spdlog/0.16.3@bincrafters/stable"
    exports_sources = "src/*", "CMakeLists.txt"
    default_options = "shared=False"
    
    def configure(self):
        self.options["sdl2"].shared = True
    
    def imports(self):
        dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        self.copy("*.dll", src="bin", dst=dest)
        self.keep_imports = True
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        
    def package(self):
        self.copy("*", src="bin",   dst="bin",    keep_path=False)
        self.copy("*.h", src="src", dst="include", keep_path=False)
        
        if self.options.shared == False:
            self.copy("*", src="lib",   dst="lib",    keep_path=False)
        
    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
