<h2 align="center">mergebot</h2>

### Build

Before we build mergebot, some tools and libs are needed.

The dependencies of Mergebot can be divided into two specific parts:

1. basic environment dependencies, such as Python and CMake. These are essential
   tools and environments required for building and running mergebot.

2. external dependency packages or frameworks, such as onetbb and Boost::Graph.
   These are additional software packages or libraries that mergebot requires
   for executing specific tasks. These dependencies may be introduced to support
   particular functionalities or perform specific computational tasks.

> **Note**
> currently mergebot can only be build and run on a Unix-like system.

#### Basic Environment

| Package | Version   | What                         | Notes                                                                                                            |
|---------|:----------|:-----------------------------|:-----------------------------------------------------------------------------------------------------------------|
| GCC     | \>=9      | C/C++ compiler               | at least gcc/g++9 is needed to support stdc++17                                                                  |
| python  | \>=3.6    | scripts and pacakge manager  | python 3.6 is needed                                                                                             |
| CMake   | \>=3.21.3 | Makefile/workspace generator | CMake is the de facto build system generator tool for C++, we use a rather new one to assure toolchains and deps |

**Installation**

+ gcc >= 9 on ubuntu:

> These commands are based on a askubuntu answer http://askubuntu.com/a/581497
> To install gcc-6 (gcc-6.1.1), I had to do more stuff as shown below.
> USE THOSE COMMANDS AT YOUR OWN RISK. I SHALL NOT BE RESPONSIBLE FOR ANYTHING.
> ABSOLUTELY NO WARRANTY.
>
>If you are still reading let's carry on with the code.
>
>sudo apt-get update && \
> sudo apt-get install build-essential software-properties-common -y && \
> sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
> sudo apt-get update && \
> sudo apt-get install gcc-snapshot -y && \
> sudo apt-get update && \
> sudo apt-get install gcc-6 g++-6 -y && \
> sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-6 60 --slave
> /usr/bin/g++ g++ /usr/bin/g++-6 && \
> sudo apt-get install gcc-4.8 g++-4.8 -y && \
> sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 60
> --slave
> /usr/bin/g++ g++ /usr/bin/g++-4.8;
>
>When completed, you must change to the gcc you want to work with by default.
> Type in your terminal:
`sudo update-alternatives --config gcc`
>
>To verify if it worked. Just type in your terminal
> gcc -v

+ python 3.6/3.7:
  Through official website of python or install a miniconda or Anaconda.

+ CMake

```shell
pip install cmake
```

+ Ninja

```shell
pip install ninja
```

#### External Dependencies

After mergebot-v0.5, we introduced conan2 to manage external dependencies.
Before using conan2 to manage external dependencies, we need to install and
initialize it:

+ Installation

```shell
pip install conan
```

+ Initialization

```shell
conan profile detect --force
```

+ Adding the Wuhan University source

```shell
conan remote add conan http://43.156.250.168:8081/artifactory/api/conan/conan
conan remote login conan oppo
```

After running `conan remote login conan oppo`, you will be prompted to enter a
password to continue. The password is: xxxxxxxxxxxxxxxx.

Once these steps are completed, we can use conan to manage our project's
dependencies.

We can install all the external libs by typing:

```shell
cd <home of mergebot>
conan install . -of=build --build=missing -r=conan
```

this command will take about 1.5 hours to build mergebot's dependencies on a
12-core Intel i5 with 32GB RAM. You can of course use machines with more cores
and higher memory capacity to accelerate this process. Fortunately, we only need
to go through this process once. In the future, we can directly build without
repeating these steps.

### Build & Run

+ Build

```shell
cd <home of mergebot>
conan install . -of=build --build=missing -r=conan
source ./build/conanbuild.sh
cmake --preset conan-release
cmake --build build
```

+ Run

```shell
# make sure you are in the home directory of mergebot
cmake --build build
```



