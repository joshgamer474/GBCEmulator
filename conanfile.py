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
                "build_type": ["Debug", "Release"],
                "cppstd": ["14", "17"]}
    options = {"shared": [True, False]}
    generators = "cmake"
    requires = (
        "sdl2/2.0.9@bincrafters/stable",
        "spdlog/1.2.1@bincrafters/stable",
        "gtest/1.8.1@bincrafters/stable",
        "libpng/1.6.34@bincrafters/stable",
        )
    exports_sources = "src/*", "CMakeLists.txt", "test_package/*"
    default_options = "shared=False"

    def configure(self):
        self.options["sdl2"].shared = True
        self.options["gtest"].shared = True
        if self.settings.os == "Linux":
            self.options["sdl2"].nas = False

    def imports(self):
        dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        self.copy("*.dll", src="bin", dst=dest)
        self.copy("*.a", src="lib", dst="lib")
        self.copy("*.so*", src="lib", dst="lib")
        #self.copy("*.lib", src="lib", dst="lib")
        self.keep_imports = True

    def build(self):
        if self.settings.os == "Linux":
            "stdc++fs"
        cmake = CMake(self, build_type=self.settings.build_type)
        cmake.configure()
        cmake.build()
        #cmake.test()

    def package(self):
        self.copy("*", src="bin",   dst="bin",    keep_path=False)
        self.copy("*.h", src="src", dst="include")

        if self.options.shared == False:
            self.copy("*", src="lib",   dst="lib",    keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
