<h2 align="center">mergebot</h2>

### Build

#### Software

| Package  |   Version    |                        Notes |
|----------|:------------:|-----------------------------:|
| CMake    |  \>=3.20.0   | Makefile/workspace generator |
| GCC      |   \>=7.1.0   |               C/C++ compiler |
| python   |    \>=3.6    |  scripts and pacakge manager |
| GNU Make | 3.79, 3.79.1 |     Makefile/build processor |

#### Deps

- [x] re2 (20230602)
- [x] nlohmann_json (build from code)
- [x] fmt (build from code)
- [x] spdlog (build from code)
- [x] magic_enum (build from code)
- [x] TBB (build from code)
- [x] Boost (bgl, build from code)

- [x] TreeSitter(tree-sitter, build from code)
- [x] tree-sitter-c (build from code)
- [x] tree-sitter-cpp (build from code)

- [x] LibGit2 (build from code)
- [x] ZLIB (installed with package manager)
- [x] Crow (revisions to be applied)
- [x] llvm (build from code)

We plan to use conan to manage all dependencies except llvm-core. We will
automate the building of other dependencies by utilizing all the other libraries
available in the Conan Center. Additionally, we will set up a conan private
server to provide the tree-sitter-cpp library.


