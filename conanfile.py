from conan import ConanFile
from conan.tools.cmake import cmake_layout

class ConanPackage(ConanFile):
    settings = "os", "arch", "compiler", "build_type"

    name = 'network-monitor'
    version = "0.1.0"

    generators = "CMakeToolchain", "CMakeDeps"

    requires = [
        ('boost/1.85.0'),
        ('openssl/3.2.1'),
        ('libcurl/8.6.0')
    ]

    default_options = {
        'boost*:shared': False,
    }

    def layout(self):
        cmake_layout(self)