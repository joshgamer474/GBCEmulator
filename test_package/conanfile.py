from conans import ConanFile, CMake
import os

class GBCEmulatorTestPackage(ConanFile):
    name = "GBCEmulatorTestPackage"
    version = "0.1"
    settings = {"os" : ["Windows"], 
                "arch": ["x86", "x86_64"],
                "compiler": ["Visual Studio"]}
    generators = "cmake"
    requires = "boost/1.66.0@conan/stable"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")

    def test(self):
        self.run("cd bin && .%s%s --log_level=test_suite" % (os.sep, self.name))
