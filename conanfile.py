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
                "lib_only": [True, False],
                "qt": [True, False]}
    generators = "cmake", "cmake_find_package"
    requires = (
        "sdl/2.0.20",
        "sdl_image/2.0.5",
        "xz_utils/5.4.0",
        "spdlog/1.9.2",
        "libpng/1.6.39",
        "libzip/1.8.0",
        )
    exports_sources = "src/*", "CMakeLists.txt", "test_package/*", "!*.gb",\
      "!*.gitignore", "!*.log", "!*.sav", "!*.s"
    default_options = "shared=False", "lib_only=False", "qt=False"

    def build_requirements(self):
        if self.settings.os == "Android":
            self.build_requires("android-ndk/r24")
        else:
            self.build_requires("gtest/1.11.0")
        if self.options.qt:
            self.build_requires("qt/5.15.8")

    def configure(self):
        self.options["sdl2"].shared = True
        self.options["gtest"].shared = True
        if self.settings.os == "Linux":
            self.options["sdl2"].iconv = False
            self.options["sdl2"].nas = False
            self.options["sdl2"].pulse = False
            self.options["sdl2"].jack = False
            self.options["sdl2"].libunwind = False
            self.options["libalsa"].shared = True

        if self.options.qt:
            self.options["qt"].shared = True
            self.options["qt"].with_sqlite3 = False
            self.options["qt"].with_mysql = False
            self.options["qt"].with_gstreamer = False
            self.options["qt"].with_odbc = False
            self.options["qt"].with_pulseaudio = False
            self.options["qt"].with_dbus = False
            #self.options["qt"].with_gssapi = False
            self.options["qt"].with_atspi = False


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
        if self.settings.os == "Android":
            self.copy("*.h", src="include", dst="include")
        if self.options.qt:
            self.copy("q*.*", src="bin/archdatadir/plugins/platforms", dst=dest + os.sep + "platforms")
            self.copy("libq*.*", src="bin/archdatadir/plugins/platforms", dst=dest + os.sep + "platforms")
        self.keep_imports = True

    def build(self):
        cmake = CMake(self, build_type=self.settings.build_type)
        # Don't build test_package as ndk doesn't have std::experimental::filesystem
        if self.settings.os == "Android":
            cmake.definitions["BUILD_UNIT_TEST"] = False
        else:
            cmake.definitions["BUILD_UNIT_TEST"] = True

        if self.options.lib_only == True or self.settings.os == "Android":
            cmake.definitions["BUILD_LIB_ONLY"] = True
        else:
            cmake.definitions["BUILD_LIB_ONLY"] = False

        if self.options.qt:
            cmake.definitions["BUILD_QT_GUI"] = True

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
