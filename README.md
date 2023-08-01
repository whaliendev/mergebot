<h2 align="center">mergebot</h2>
<p align="center">a structured git conflicts analysis tool</p>

<a href="./docs/README.zh-CN.md">简体中文</a>&nbsp;|&nbsp;<a href="./README.md">
English</a>

### Build

> **Note:**
>
> mergebot can only be built and run on Unix-like systems.

#### Dependencies

Before building mergebot, you need to install some tools and libraries.

mergebot has two categories of dependencies:

1. Base environment, such as Python and CMake. These are essential tools and
   environments required for building and running modern C++ projects.

2. External dependency packages or frameworks, such as onetbb and Boost::Graph.
   These are additional software packages or libraries needed for mergebot to
   perform specific tasks.

**1. Base Environment**

| Package | Version   | What                         | Notes                                                                                                                              |
|---------|-----------|------------------------------|------------------------------------------------------------------------------------------------------------------------------------|
| GCC     | \>=9      | C/C++ compiler               | mergebot requires a C++17 compliant compiler for building                                                                          |
| python  | \>=3.6    | scripts and package manager  | We use Python and conan2 for managing third-party dependencies                                                                     |
| CMake   | \>=3.21.3 | Makefile/workspace generator | CMake is the de facto standard build system generator for C++ projects. We use a relatively new version to utilize modern features |

**Installation**

+ Install gcc >= 9 on Ubuntu 16.04

> The following commands are taken
> from: [askubuntu](http://askubuntu.com/a/581497)
>
> ```shell
> sudo apt-get update && \
> sudo apt-get install build-essential software-properties-common -y && \
> sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
> sudo apt-get update && \
> sudo apt-get install gcc-snapshot -y && \
> sudo apt-get update && \
> sudo apt-get install gcc-6 g++-6 -y && \
> sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-6 60 --slave /usr/bin/g++ g++ /usr/bin/g++-6 && \
> sudo apt-get install gcc-4.8 g++-4.8 -y && \
> sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.8
> ```
>
> After completion, you must switch to the desired gcc version by entering the
> following in the terminal:
> `sudo update-alternatives --config gcc`
>
> To verify if the switch was successful, simply type in the terminal:
> `gcc -v`

+ Python 3.6/3.7:

Install from the [Python official website](https://www.python.org/downloads/),
or install [Miniconda](https://docs.conda.io/en/main/miniconda.html) or
[Anaconda](https://www.anaconda.com/download).

+ CMake:

```shell
pip install cmake
```

+ Ninja:

```shell
pip install ninja
```

2. External Dependencies

Starting from mergebot-v0.5, we introduced conan2 to manage external
dependencies.
Before using conan2 to manage external dependencies, we need to install and
initialize it:

+ Installation

```shell
pip install conan
```

+ Initialization

Generate a default conan2 profile first:

```shell
conan profile detect --force
```

Then, modify the C++ compiler standard to gnu17 in the default profile of
conan2:

```shell
sed -i 's/compiler.cppstd=[^ ]*/compiler.cppstd=gnu17/' ~/.conan2/profiles/default
```

+ Adding the WHU conan repository

```shell
conan remote add conan http://43.156.250.168:8081/artifactory/api/conan/conan
conan remote login conan oppo
```

After running `conan remote login conan oppo`, you will be prompted to enter the
password to continue. The password is: xxxxxxxxxxxxxxxx.

Once these steps are completed, we can use conan to manage the project's
dependencies.

We can install all external libraries using the following command:

```shell
cd <mergebot's base directory>
conan install . --build=missing -r=conan -r=conancenter -s build_type=[Release | RelWithDebInfo | Debug | MinSizeRel]
```

On a machine with a 6-core Intel i5 and 32GB RAM, this command takes
approximately 1.5 hours to install mergebot's dependencies. Of course, you can
use a machine with more cores and larger memory capacity to speed up this
process. Fortunately, we only need to perform this process once, and subsequent
builds will not take as long.

#### Build

```shell
cmake --preset [conan-debug | conan-release]
cmake --build build/[Debug | Release]
```

### Run or Package

+ Run

If all you want is to run the program during development:

```shell
source build/[Debug | Release]/generators/conanrun.sh
./build/[Debug | Release]/bin/mergebot
```

+ Package

After a successful build, the `{MB_BIN_DIR}` will be populated with utility
scripts and mergebot binaries. You can directly
package or move this directory to deploy it on any Unix system with the same
distribution and major version by running:

```shell
cd {mergebot dir}/build/[Debug | Release]/bin
./mergebot.run
```

> **Notes**
>
> When you need to deploy to production, it is recommended to set
> the `build_type` to `Release` in the `conan install ...` command mentioned
> above.
