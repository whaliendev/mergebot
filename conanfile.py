import os
import shutil

from conan import ConanFile
from conan.api.output import ConanOutput
from conan.tools.cmake import cmake_layout, CMakeToolchain, CMakeDeps, CMake


class MergebotConan(ConanFile):
    name = "mergebot-sa"
    version = "0.5-beta"
    license = "Apache-2.0"
    author = "Hwa He (hwahe.cs@gmail.com)"
    url = "https://github.com/conan-io/conan-center-index"
    description = "auto resolve merge conflicts for C/C++ semantically"
    topics = ("git", "vcs", "merge", "conflicts", "semantic")

    package_type = "application"

    settings = "os", "compiler", "build_type", "arch"

    options = {
        "fPIC": [True, False],
    }

    requires = (
        "re2/20230201",
        "nlohmann_json/3.11.2",
        "fmt/9.1.0",
        # the revision is appended to make spdlog 1.11 depends on fmt 9.1.0
        "spdlog/1.11.0#d0fdbaa523550b89156084bf42b41c90",
        "magic_enum/0.9.2",
        "onetbb/2021.9.0",
        "boost/1.81.0",
        "tree-sitter/0.20.8",
        "tree-sitter-c/0.20.3",
        "tree-sitter-cpp/0.20.0",
        "libgit2/1.7.0",
        # "zlib/[>=1.2.10]",
        "cstar-crow/4f3f5de",
        "llvm/16.0.6",
        "zstd/1.5.5"
    )

    default_options = {
        "fPIC": True,

        "re2/*:shared": False,
        # enable fmt to be referenced
        "fmt/*:shared": True,
        "spdlog/*:shared": False,
        "onetbb/*:shared": True,
        "boost/*:shared": False,
        "tree-sitter/*:shared": True,
        "tree-sitter-c/*:shared": True,
        "tree-sitter-cpp/*:shared": True,
        "libgit2/*:shared": False,
        # "zlib/*:shared": False,

        "re2/*:fPIC": True,
        "fmt/*:fPIC": True,
        "spdlog/*:fPIC": True,
        "onetbb/*:fPIC": True,
        "boost/*:fPIC": True,
        "tree-sitter/*:fPIC": True,
        "tree-sitter-c/*:fPIC": True,
        "tree-sitter-cpp/*:fPIC": True,
        "libgit2/*:fPIC": True,
        # "zlib/*:fPIC": True,

        "re2/*:with_icu": False,

        "fmt/*:header_only": False,
        "fmt/*:with_fmt_alias": False,
        "fmt/*:with_os_api": True,

        "spdlog/*:header_only": False,
        "spdlog/*:wchar_support": False,
        "spdlog/*:wchar_filenames": False,
        "spdlog/*:no_exceptions": False,

        "onetbb/*:tbbmalloc": True,
        "onetbb/*:tbbproxy": True,
        "onetbb/*:interprocedural_optimization": True,

        # for boost, we apply default options, remove unnecessary dependencies
        "boost/*:without_graph": False,
        "boost/*:without_math": False,
        "boost/*:without_random": False,
        "boost/*:without_regex": False,
        "boost/*:without_serialization": False,
        "boost/*:without_system": False,

        "boost/*:without_atomic": True,
        "boost/*:without_chrono": True,
        "boost/*:without_container": True,
        "boost/*:without_context": True,
        "boost/*:without_contract": True,
        "boost/*:without_coroutine": True,
        "boost/*:without_date_time": True,
        "boost/*:without_exception": True,
        "boost/*:without_fiber": True,
        "boost/*:without_filesystem": True,
        "boost/*:without_graph_parallel": True,
        "boost/*:without_iostreams": True,
        "boost/*:without_json": True,
        "boost/*:without_locale": True,
        "boost/*:without_log": True,
        "boost/*:without_mpi": True,
        "boost/*:without_nowide": True,
        "boost/*:without_program_options": True,
        "boost/*:without_python": True,
        "boost/*:without_stacktrace": True,
        "boost/*:without_test": True,
        "boost/*:without_thread": True,
        "boost/*:without_timer": True,
        "boost/*:without_type_erasure": True,
        "boost/*:without_url": True,
        "boost/*:without_wave": True,

        "libgit2/*:threadsafe": True,
        "libgit2/*:with_iconv": False,
        "libgit2/*:with_libssh2": True,
        "libgit2/*:with_https": "openssl",
        "libgit2/*:with_sha1": "collisiondetection",
        "libgit2/*:with_ntlmclient": True,
        "libgit2/*:with_regex": "builtin",

        "crowcpp-crow/*:amalgamation": False,
        "crowcpp-crow/*:with_ssl": False,
        "crowcpp-crow/*:with_compression": True,

        "llvm/*:with_project_clang": True,
        "llvm/*:with_runtime_libcxx": False,
        "llvm/*:with_runtime_libcxxabi": False,
        "llvm/*:with_runtime_libunwind": False,
        "llvm/*:shared": False,
        "llvm/*:llvm_build_llvm_dylib": False,
        "llvm/*:llvm_link_llvm_dylib": False,
        "llvm/*:exceptions": False,
        "llvm/*:rtti": False,
        # it's very time-consuming to enable lto
        "llvm/*:lto": 'Off',
        # set it to False is mandatory, or it will fail on some platforms
        "llvm/*:with_z3": False,
        "llvm/*:with_zlib": True,
        # fails with gcc9 on ubuntu
        "llvm/*:with_xml2": False,
        # the recipe needs gcc10 to build llvm, while C++17 is supported in gcc9
        "llvm/*:enable_unsafe_mode": True,
        "llvm/*:conan_center_index_limits": False
    }

    _mergebot_scripts = [
        'mergebot.run',
        'setup.sh'
    ]

    _mergebot_docs = [
        'README.zh-CN.md',
        'api-mergebot-sa_v1.1.md'
    ]

    def layout(self):
        cmake_layout(self)

    def _package_scripts(self):
        scripts_folder = os.path.join(self.source_folder, "scripts")
        bin_out_folder = os.path.join(self.build_folder, "bin")
        if not os.path.exists(bin_out_folder):
            os.makedirs(bin_out_folder)

        for script in self._mergebot_scripts:
            source_file = os.path.join(scripts_folder, script)
            dest_file = os.path.join(bin_out_folder, script)
            ConanOutput(str(self)).info(f'copying {source_file} '
                                        f'to {{MB_BIN_DIR}}/{script}')
            shutil.copy2(source_file, dest_file)

    def _package_docs(self):
        docs_folder = os.path.join(self.source_folder, "docs")
        bin_out_folder = os.path.join(self.build_folder, "bin")
        if not os.path.exists(bin_out_folder):
            os.makedirs(bin_out_folder)

        for doc in self._mergebot_docs:
            source_file = os.path.join(docs_folder, doc)
            dest_file = os.path.join(bin_out_folder, doc)
            ConanOutput(str(self)).info(f'copying {source_file} '
                                        f'to {{MB_BIN_DIR}}/{doc}')
            shutil.copy2(source_file, dest_file)

    def generate(self):
        tc = CMakeToolchain(self, "Ninja")
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

        # package misc files
        self._package_scripts()
        self._package_docs()

    def build(self):
        cmake = CMake(self)
        cmake.build()

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.21.3 <4.0.0]")
        self.tool_requires("ninja/[>=1.10.0 <2.0.0]")
