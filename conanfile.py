from conan import ConanFile

class ConanPackage(ConanFile):
    settings = "os", "arch", "compiler", "build_type"

    name = 'network-monitor'
    version = "0.1.0"

    generators = 'CMakeDeps'

    requires = [
    ]