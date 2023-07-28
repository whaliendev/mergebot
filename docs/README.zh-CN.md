<h2 align="center">mergebot</h2>
<p align="center">一个结构化的git合并冲突分析工具</p>

<a href="./README.zh-CN.md">简体中文</a>&nbsp;|&nbsp;<a href="../README.md">
English</a>

### 构建

> **注意:**
>
> mergebot 当前只能在类Unix系统上构建和运行。

#### 依赖

在构建 mergebot 之前，需要安装一些工具和库。

mergebot 的依赖可以分为两个方面：

+ 基础环境，例如 Python 和 CMake。这些是构建和运行现代 C++ 项目所必需的基本工具和环境。

+ 外部依赖包或框架，例如 onetbb 和 Boost::Graph。这些是 mergebot
  执行特定任务所需的额外软件包或库。

**1. 基础环境**

| Package | Version   | What                         | Notes                                        |
|---------|-----------|------------------------------|----------------------------------------------|
| GCC     | \>=9      | C/C++ compiler               | mergebot需要完全支持C++17标准的编译器来构建                 |
| python  | \>=3.6    | scripts and pacakge manager  | 我们使用 Python 编写的conan2来管理第三方依赖包               |
| CMake   | \>=3.21.3 | Makefile/workspace generator | CMake 是 C++ 的事实标准构建系统生成工具，我们使用相对较新的版本以使用现代特性 |

**安装**

+ 在Ubuntu 16.04 上安装 gcc >= 9

> 以下命令来自：[askubuntu](http://askubuntu.com/a/581497)
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
> 完成后，您必须默认切换到您要使用的 gcc 版本。
> 在终端中输入：
> `sudo update-alternatives --config gcc`
>
> 要验证是否成功，只需在终端中输入
> `gcc -v`

+ python 3.6/3.7:

通过 [Python 官方网站](https://www.python.org/downloads/)
安装，或者安装 [Miniconda](https://docs.conda.io/en/main/miniconda.html)
或 [Anaconda](https://www.anaconda.com/download)。

+ CMake:

```shell
pip install cmake
```

+ Ninja:

```shell
pip install ninja
```

2. 外部依赖

从 mergebot-v0.5 开始，我们引入了 conan2 来管理外部依赖项。
在使用 conan2 管理外部依赖之前，我们需要安装并初始化它：

+ 安装

```shell
pip install conan
```

+ 初始化

```shell
conan profile detect --force
```

+ 添加 WHU conan 存储库

```shell
conan remote add conan http://43.156.250.168:8081/artifactory/api/conan/conan
conan remote login conan oppo
```

在运行 `conan remote login conan oppo` 后，您将被提示输入密码以继续。密码为：xxxxxxxxxxxxxxxx。

完成这些步骤后，我们可以使用 conan 来管理项目的依赖关系。

我们可以通过以下命令安装所有外部库：

```shell
cd <mergebot 的基础目录>
conan install . --build=missing -r=conan -s
build_type=[Release | RelWithDebInfo | Debug | MinSizeRel]
```

在具有 6核12线程的Intel i5 和 32GB RAM 的机器上，该命令大约需要 1.5 小时来安装
mergebot 的依赖项。您当然也可以使用具有更多核心和更大内存容量的机器来加快此过程。幸运的是，我们只需要执行此过程一次，后续再次构建无需再安装依赖。

#### 构建与运行

+ 构建

```shell
cmake --preset [conan-debug | conan-release]
cmake --build build/[Debug | Release]
```

+ 运行

```shell
source build/[Debug | Release]/generators/conanrun.sh
./build/[Debug | Release]/bin/mergebot
```
