from conans import ConanFile, CMake
import os

class GBCEmulator_qt(ConanFile):

    name = "GBCEmulator_qt"
    version = "0.0.1"
    url = "https://github.com/joshgamer474/GBCEmulator"
    description = "A WIP Gameboy (Color) emulator written in C++"
    settings = {"os" : ["Windows", "Linux"], 
                "arch": ["x86", "x86_64"],
                "compiler": ["Visual Studio", "gcc"],
                "build_type": ["Debug", "Release"],
                "cppstd": ["14"]}
    options = {"shared": [True, False]}
    generators = "cmake"
    requires = (
        "GBCEmulator/0.0.2@josh/testing",
        "sdl2/2.0.8@bincrafters/stable",
        "qt/5.12.3@bincrafters/stable",
        "spdlog/1.2.1@bincrafters/stable")
    exports_sources = "src/*", "CMakeLists.txt"
    default_options = "shared=True"
    
    def configure(self):
        self.options["sdl2"].shared = True
        self.options["sld2"].nas = False

    def imports(self):
        bin_dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        lib_dest = os.getenv("CONAN_IMPORT_PATH", "lib")
        self.copy("Qt*.dll", src="bin", dst=bin_dest)
        self.copy("SDL*.dll", src="bin", dst=bin_dest)
        self.copy("qwindows*.dll", src="plugins/platforms", dst=bin_dest + os.sep + "platforms")
        self.copy("*.so*", src="lib", dst=lib_dest)
        self.copy("*.a", src="lib", dst=lib_dest)
        if self.options.shared == False:
            self.copy("*.lib", src="lib", dst=lib_dest)
        self.keep_imports = True
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        
    def package(self):
        self.copy("*", src="bin", dst="bin",    keep_path=False)
        self.copy("*GBCEmulator.*", src="lib", dst="lib")
        self.copy("*.h", src="src", dst="include", keep_path=False)
        
    def package_info(self):
        self.cpp_info.libs.append(self.name)
