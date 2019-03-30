from conans import ConanFile, CMake, tools
import os

class GBCEmulator(ConanFile):

    name = "GBCEmulator"
    version = "0.0.2"
    url = "https://github.com/joshgamer474/GBCEmulator"
    description = "A WIP Gameboy (Color) emulator written in C++"
    settings = {"os" : ["Windows", "Linux"], 
                "arch": ["x86", "x86_64"],
                "compiler": ["Visual Studio", "gcc"],
                "build_type": ["Debug", "Release"]}
    options = {"shared": [True, False]}
    generators = "cmake"
    requires = "sdl2/2.0.9@bincrafters/stable", \
    requires = "sdl2/2.0.9@bincrafters/stable", \
                "spdlog/1.2.1@bincrafters/stable"
    exports_sources = "src/*", "CMakeLists.txt"
    default_options = "shared=False"
    
    def configure(self):
        self.options["sdl2"].shared = True
        self.options["sdl2"].nas = False
    
    def imports(self):
        dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        self.copy("*.dll", src="bin", dst=dest)
        self.copy("*.a", src="lib", dst="lib")
        self.copy("*.so*", src="lib", dst="lib")
        self.keep_imports = True
    
    def build(self):
        cmake = CMake(self, build_type=self.settings.build_type)
        cmake.configure()
        cmake.build()
        
    def package(self):
        self.copy("*", src="bin",   dst="bin",    keep_path=False)
        self.copy("*.h", dst="include", keep_path=True)
        
        if self.options.shared == False:
            self.copy("*", src="lib",   dst="lib",    keep_path=False)
        
    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
