from conan import ConanFile
from conan.tools.cmake import cmake_layout

class WingzEngineConanFile(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    name = "wingz-engine"
    version = "0.1.0"

    def requirements(self):
        self.requires("glfw/3.3.8")
        self.requires("entt/3.12.2")
        self.requires("spdlog/1.12.0")

    def layout(self):
        cmake_layout(self)
