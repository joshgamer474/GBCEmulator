from conan import ConanFile
from conan.tools.files import copy, save, load
from conan.tools.gnu import AutotoolsToolchain, AutotoolsDeps
from conan.tools.microsoft import unix_path, VCVars, is_msvc
from conan.errors import ConanInvalidConfiguration
from conan.errors import ConanException
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
import os

class GBCEmulator(ConanFile):

    name = "gbcemulator"
    version = "0.1.5"
    url = "https://github.com/joshgamer474/GBCEmulator"
    description = "A WIP Gameboy (Color) emulator written in C++"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False],
                "lib_only": [True, False],
                "qt": [True, False]}
    requires = (
        "sdl/2.30.9",
        "spdlog/1.9.2",
        "libpng/1.6.39",
        "libzip/1.8.0",
        )
    exports_sources = "src/*", "CMakeLists.txt", "test_package/*", "!*.gb",\
      "!*.gitignore", "!*.log", "!*.sav", "!*.s"
    default_options = {"shared": False, "lib_only": False, "qt": False}

    def build_requirements(self):
        if self.settings.os == "Android":
            self.tool_requires("android-ndk/r24")
        else:
            self.test_requires("gtest/1.16.0")
        if self.options.qt:
            self.tool_requires("qt/5.15.8")

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

    def layout(self):
        cmake_layout(self)

    def generate(self):
        # This generates "conan_toolchain.cmake" in self.generators_folder
        tc = CMakeToolchain(self)
        tc.variables["MYVAR"] = "1"
        tc.preprocessor_definitions["MYDEFINE"] = "2"

        # Don't build test_package as ndk doesn't have std::experimental::filesystem
        if self.settings.os == "Android":
            tc.variables["BUILD_UNIT_TEST"] = False
        else:
            #tc.variables["BUILD_UNIT_TEST"] = True
            tc.variables["BUILD_UNIT_TEST"] = False

        if self.options.lib_only == True or self.settings.os == "Android":
            tc.variables["BUILD_LIB_ONLY"] = True
        else:
            tc.variables["BUILD_LIB_ONLY"] = False

        if self.options.qt:
            tc.variables["BUILD_QT_GUI"] = True

        tc.generate()

        # This generates "foo-config.cmake" and "bar-config.cmake" in self.generators_folder
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        #cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()
#        libDest = os.getenv("CONAN_IMPORT_PATH", "lib")
#        if (self.settings.arch == "armv7"):
#            libDest += os.sep + "armeabi-v7a"
#        elif (self.settings.arch == "armv8"):
#            libDest += os.sep + "arm64-v8a"
#        else:
#            libDest += os.sep + str(self.settings.arch)
#
#        # Define output dirs
#        binDest = join(self.package_folder, "bin")
#        libDest = join(self.package_folder, "lib")
#        includeDest = join(self.package_folder, "include")
#
#        copy(self, "GBCEmulator*", self.build_folder, binDest, keep_path=False, excludes="GBCEmulatorTest*")
#        copy(self, "*.dll", self.build_folder, binDest, excludes="g*.dll")
#        copy(self, "*.h", self.source_folder, includeDest)
#        copy(self, "*.a", self.build_folder, libDest, keep_path=False)
#        copy(self, "*.so", self.build_folder, libDest, keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)

    def deploy(self):
        if self.options.lib_only == True:
            self.copy("*GBCEmulator*", dst="lib", src="lib")
        else:
            self.copy("*", dst="bin", src="bin")
            self.copy("*", dst="lib", src="lib")

        self.copy("*", dst="include", src="include")
