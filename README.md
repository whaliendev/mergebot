<h2 style="align: center; ">mergebot</h2>

### Develop
#### Source tree

#### Dependencies
- python >= 3.6
At least python3.6 is needed to support modern C++ package manager `conan`
- conan 
We use `conan` to manage 3rd-party library dependencies.
- cmake
At least cmake3.15 is needed to generate the build system. 
- a set of compiler toolchain which supports C++17  
    + gcc >= 7.1
    + clang >= 5
    + currently we don't promise the portability of MSVC toolchains
- a build system is needed to build this project. For example, `Unix Makefile`, 
`Ninja`, `Visual Studio toolchains`, etc. 

#### Optional Dependencies
To contribute to this project, we need following dev dependencies to unify 
the coding style of our project
- clang-format
we use `clang-format` to unify the coding style of this project.  
- clang-tidy
`clang-tidy` is our linter tool. 
- pre-commit
this is a dev dependency to manage the git commit hooks, which can assure 
our linter tool to be executed at git pre-commit stage.  

#### Howto
- conan 
```shell
conan profile new default --detect
conan profile update settings.compiler.libcxx=libstdc++11 default
```


