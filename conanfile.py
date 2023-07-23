from conan import ConanFile
from conan.tools.scm.git import Git


class MergebotConan(ConanFile):
    name = "mergebot-sa"
    version = "0.1"
    license = "[Apache-2.0](https://www.apache.org/licenses/LICENSE-2.0)"
    author = "Hwa He (hwahe.cs@gmail.com)"
    url = "https://github.com/whaliendev/mergebot"
    description = "auto resolve merge conflicts for C/C++ semantically"
    topics = ("git", "vcs", "merge", "conflicts", "semantic")

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }

    requires = [
        "re2/20230602",
        "nlohmann_json/3.11.2",
        "fmt/9.1.0",
        "spdlog/1.11.0",
        "magic_enum/0.9.2",
        "onetbb/2021.9.0",
        "boost/[>=1.80.0]",
        "tree-sitter/0.20.6",
        "libgit2/1.5.0",
        "zlib/[>=1.2.10]",
    ]

    default_options = {
        "re2:shared": False,
        "fmt2:shared": False,
        "spdlog:shared": False,
        "onetbb:shared": False,
        "boost:shared": False,
        "tree-sitter:shared": False,
        "libgit2:shared": False,
        "zlib:shared": False,

        "re2:fPIC": True,
        "fmt:fPIC": True,
        "spdlog:fPIC": True,
        "onetbb:fPIC": True,
        "boost:fPIC": True,
        "tree-sitter:fPIC": True,
        "libgit2:fPIC": True,
        "zlib:fPIC": True,

        "re2:with_icu": False,
        "fmt:header_only": False,

        "fmt:with_fmt_alias": False,
        "fmt:with_os_api": True,

        "spdlog:header_only": False,
        "spdlog:wchar_support": False,
        "spdlog:wchar_filenames": False,
        "spdlog:no_exceptions": False,

        "onetbb:tbbmalloc": True,
        "onetbb:tbbproxy": True,
        "onetbb:interprocedural_optimization": True,

        # for boost, we apply default options, remove unnecessary dependencies
        "boost:without_graph": False,
        "boost:without_regex": False,
        "boost:without_atomic": True,
        "boost:without_chrono": True,
        "boost:without_container": True,
        "boost:without_context": True,
        "boost:without_contract": True,
        "boost:without_coroutine": True,
        "boost:without_date_time": True,
        "boost:without_exception": True,
        "boost:without_fiber": True,
        "boost:without_filesystem": True,
        "boost:without_graph_parallel": True,
        "boost:without_iostreams": True,
        "boost:without_json": True,
        "boost:without_locale": True,
        "boost:without_log": True,
        "boost:without_math": True,
        "boost:without_mpi": True,
        "boost:without_nowide": True,
        "boost:without_program_options": True,
        "boost:without_python": True,
        "boost:without_random": True,
        "boost:without_serialization": True,
        "boost:without_stacktrace": True,
        "boost:without_system": True,
        "boost:without_test": True,
        "boost:without_thread": True,
        "boost:without_timer": True,
        "boost:without_type_erasure": True,
        "boost:without_url": True,
        "boost:without_wave": True,
    }

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    # def configure(self):
    #     if self.options.shared:
    #         self.options.rm_safe("fPIC")

    def build_requirements(self):
        self.system_requires("cmake/[>=3.26.4]")
        self.system_requires("ninja/[>=1.10.0]")
