## MergeBot Platform Build Guide

MergeBot is a client-server platform designed for large-scale C/C++ merge conflict resolution. It consists of three main components:

- **Interactive Web App**: Simplifies user interaction and minimizes errors during conflict resolution.
- **Backend Server**: Manages Git repositories and metadata for conflict merge scenarios.
- **Algorithm Server (MergeSyn)**: Utilizes static analysis and program synthesis to suggest conflict resolutions for C/C++ code.

### Quick Start

For a quick overview of the MergeBot platform, use the [Docker Compose setup](../docker-compose.yml) and follow the [Usage Guide](../README.md) in the main README to run the platform locally.

**Note**:
- MergeBot supports only Unix-style paths. It is recommended to use macOS or Linux for deployment and usage.
- On Windows, you can use the Windows Subsystem for Linux (WSL) or a virtual machine to run MergeBot.

### MergeSyn

#### Dependencies

Before building MergeSyn, install the required tools and libraries. MergeSyn has two categories of dependencies:

1. **Base Environment**: Essential tools and environments for building and running modern C++ projects.
2. **External Dependencies**: Additional software packages or libraries needed for specific tasks.

##### 1. Base Environment

| Package | Version   | Description                   | Notes                                                                                           |
|---------|-----------|-------------------------------|-------------------------------------------------------------------------------------------------|
| GCC     | ≥9        | C/C++ Compiler                | Requires a C++17 compliant compiler.                                                           |
| Python  | ≥3.6      | Scripts and Package Manager   | Python and Conan 2 are used for managing third-party dependencies.                             |
| CMake   | ≥3.21.3   | Build System Generator        | Utilizes a recent version to leverage modern features.                                         |
| Ninja   | Latest    | Build System Tool             | Used for faster builds.                                                                        |

###### Installation

**GCC ≥9 on Ubuntu 16.04**

Execute the following commands to install GCC version 9 or higher:

```shell
sudo apt-get update && \
sudo apt-get install build-essential software-properties-common -y && \
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
sudo apt-get update && \
sudo apt-get install gcc-9 g++-9 -y && \
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-9
```

After installation, switch to the desired GCC version:

```shell
sudo update-alternatives --config gcc
```

Verify the GCC version:

```shell
gcc -v
```

**Python 3.6/3.7**

Install Python from the [official website](https://www.python.org/downloads/) or use [Miniconda](https://docs.conda.io/en/main/miniconda.html) or [Anaconda](https://www.anaconda.com/download).

**CMake**

Install CMake using pip:

```shell
pip install cmake
```

**Ninja**

Install Ninja using pip:

```shell
pip install ninja
```

##### 2. External Dependencies

Starting from MergeSyn v0.5, Conan 2 is used to manage external dependencies.

###### Installation and Initialization

**Install Conan**

```shell
pip install conan
```

**Initialize Conan**

Generate a default Conan profile:

```shell
conan profile detect --force
```

Set the C++ standard to `gnu17` in the default profile:

```shell
sed -i 's/compiler.cppstd=[^ ]*/compiler.cppstd=gnu17/' ~/.conan2/profiles/default
```

**Add the WHU Conan Repository**

```shell
conan remote add conan http://43.156.250.168:8081/artifactory/api/conan/conan
conan remote login conan oppo
```

When prompted, mail to `aHVhaGVAd2h1LmVkdS5jbgo=` (Base64 encoded) for the password.

###### Install Dependencies

Navigate to the project root directory and install dependencies:

```shell
conan install . --build=missing -r=conan -r=conancenter -s build_type=[Release | Debug]
```

**Note**: On a machine with a 6-core Intel i5 and 32GB RAM, this command takes approximately 1.5 hours. Using a machine with more cores and memory can speed up the process. This step is required only once; subsequent builds will be faster.

#### Building MergeSyn

Use CMake presets to configure and build MergeSyn:

```shell
cmake --preset [conan-debug | conan-release]
cmake --build build/[Debug | Release]
```

#### Running or Packaging

##### Run

Run the built binaries directly from the build directory:

```shell
./build/[Debug | Release]/bin/mergebot
```

##### Package

After a successful build, the `{MB_BIN_DIR}` directory will contain utility scripts and MergeBot binaries. To deploy on another Unix system with the same distribution and major version:

1. Navigate to the binaries directory:

    ```shell
    cd {mergebot_dir}/build/[Debug | Release]/bin
    ./setup.sh
    ```

2. Copy and archive the `{MB_BIN_DIR}` directory.

3. On the target machine, run MergeBot with the appropriate `LD_LIBRARY_PATH`:

    ```shell
    LD_LIBRARY_PATH={path_to_MB_BIN_DIR}:{path_to_MB_BIN_DIR}/dylib ./mergebot
    ```

**Note**: For production deployment, set the `build_type` to `Release` in the `conan install` command:

```shell
conan install . --build=missing -r=conan -r=conancenter -s build_type=Release
```

### Frontend

For instructions on building and running the frontend, refer to the [Frontend README](../ui/frontend/README.md).

### Backend

For instructions on building and running the backend, refer to the [Backend README](../ui/conflict-manager/README.md).

---

For further assistance, please refer to the project's [Documentation](../README.md) or open an issue on the [GitHub repository](https://github.com/whaliendev/mergebot).