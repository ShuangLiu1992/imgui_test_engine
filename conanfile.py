from conan import ConanFile
import conan.tools.files
from conan.tools.cmake import CMake, CMakeToolchain


class ImguiTestEngineConan(ConanFile):
    name = "imgui_test_engine"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    generators = "CMakeDeps"
    exports_sources = "CMakeLists.txt",

    def requirements(self):
        self.requires(f"imgui/tag_docking_6.1", transitive_headers=True)

    def source(self):
        url = self.conan_data["sources"]["url"].format(version=self.version)
        conan.tools.files.get(self, url,
                            **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS"] = 1
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.install()
