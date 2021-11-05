from conans import ConanFile, CMake, tools
import os

class GBCEmulator(ConanFile):

    name = "GBCEmulator"
    version = "0.1.4"
    url = "https://github.com/joshgamer474/GBCEmulator"
    description = "A WIP Gameboy (Color) emulator written in C++"
    settings = {"os" : ["Windows", "Linux", "Android", "Macos"], 
                "arch": ["x86", "x86_64", "armv7", "armv8"],
                "compiler": ["Visual Studio", "gcc", "clang", "apple-clang"],
                "build_type": ["Debug", "Release"]}
    options = {"shared": [True, False],
                "lib_only": [True, False]}
    generators = "cmake", "cmake_find_package"
    requires = (
        "sdl/2.0.16",
        "spdlog/1.9.2",
        "libpng/1.6.37",
        "libzip/1.7.3"
        )
    exports_sources = "src/*", "CMakeLists.txt", "test_package/*"
    default_options = "shared=False", "lib_only=False"

    def build_requirements(self):
        if self.settings.os == "Android":
            self.build_requires("android_ndk_installer/r19c@bincrafters/stable")
        else:
            self.build_requires("gtest/1.11.0")

    def configure(self):
        self.options["sdl2"].shared = True
        self.options["gtest"].shared = True
        if self.settings.os == "Linux":
            self.options["sdl2"].iconv = False
            self.options["sdl2"].nas = False
            self.options["sdl2"].pulse = False
            self.options["sdl2"].jack = False
            self.options["libalsa"].shared = True

        self.options["libzip"].shared = True
        self.options["libzip"].with_bzip2 = False
        self.options["libzip"].with_lzma = False
        self.options["libzip"].with_zstd = False
        self.options["libzip"].crypto = False

    def imports(self):
        dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        libDest = os.getenv("CONAN_IMPORT_PATH", "lib")
        if self.settings.os != "Windows":
          libDest += os.sep + str(self.settings.arch)
        self.copy("*.dll", src="bin", dst=dest)
        self.copy("*.dylib", src="lib", dst=libDest)
        self.copy("*.so*", src="lib", dst=libDest)
        if (self.settings.os == "Android"):
            self.copy("*.h", src="include", dst="include")
        self.keep_imports = True

    def build(self):
        cmake = CMake(self, build_type=self.settings.build_type)
        # Don't build test_package as ndk doesn't have std::experimental::filesystem
        if self.settings.os == "Android":
            cmake.definitions["BUILD_UNIT_TEST"] = False
        else:
            cmake.definitions["BUILD_UNIT_TEST"] = True

        if self.options.lib_only == True:
            cmake.definitions["BUILD_LIB_ONLY"] = True
        else:
            cmake.definitions["BUILD_LIB_ONLY"] = False

        cmake.configure()
        cmake.build()
        #cmake.test()

    def package(self):
        libDest = os.getenv("CONAN_IMPORT_PATH", "lib")
        if (self.settings.arch == "armv7"):
            libDest += os.sep + "armeabi-v7a"
        elif (self.settings.arch == "armv8"):
            libDest += os.sep + "arm64-v8a"
        else:
            libDest += os.sep + str(self.settings.arch)
        self.copy("GBCEmulator*", src="bin", dst="bin", keep_path=False, excludes="GBCEmulatorTest*")
        self.copy("*.dll", src="bin", dst="bin", excludes="g*.dll")
        self.copy("*.h", src="src", dst="include")
        self.copy("*.h", src="include", dst="include")
        self.copy("*.a", src="lib", dst=libDest, keep_path=False)
        self.copy("*.so", src="lib", dst=libDest, keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)

    def deploy(self):
        if self.options.lib_only == True:
            self.copy("*GBCEmulator*", dst="lib", src="lib")
        else:
            self.copy("*", dst="bin", src="bin")
            self.copy("*", dst="lib", src="lib")

        self.copy("*", dst="include", src="include")
