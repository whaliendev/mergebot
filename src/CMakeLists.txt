# we don't use conan actually
find_package(Crow REQUIRED)
find_package(re2 REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(magic_enum REQUIRED)
find_package(Clang REQUIRED)
find_package(TBB REQUIRED)
find_package(Boost 1.71 REQUIRED
        COMPONENTS
        graph)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(TreeSitter MODULE REQUIRED)
find_package(LibGit2 MODULE REQUIRED)

if (Clang_FOUND)
    message(STATUS " Found Clang ${Clang_PACKAGE_VERSION} ")
    message(STATUS " Clang found at ${Clang_DIR} ")
    include_directories(${CLANG_INCLUDE_DIRS})
endif (Clang_FOUND)

find_package(LLVM REQUIRED CONFIG)
message(STATUS " Found LLVM ${LLVM_PACKAGE_VERSION} ")
message(STATUS " Using LLVMConfig.cmake in: ${LLVM_DIR} ")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs support)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ./*.cpp ./*.hpp)
list(FILTER SRC_FILES EXCLUDE REGEX "core/.*")
# if llvm compiles with -fno-rtti, all dependent srcs should compile with -fno-rtti
file(GLOB_RECURSE RTTI_DISABLED_SRCS CONFIGURE_DEPENDS ./core/*.cpp ./core/*.hpp)
foreach (file ${RTTI_DISABLED_SRCS})
    set_source_files_properties(${file} PROPERTIES COMPILE_FLAGS "-fno-rtti")
endforeach ()

# extract src files for test purpose
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/server.cpp)

add_library(${PROJECT_NAME}-dep SHARED ${SRC_FILES} ${RTTI_DISABLED_SRCS})
target_link_libraries(${PROJECT_NAME}-dep
        INTERFACE
        clang-cpp
        fmt::fmt
        git2::git2
        ${llvm_libs}
        magic_enum::magic_enum
        re2::re2
        TBB::tbb
        tree-sitter::tree-sitter
        tree-sitter::tree-sitter-cpp
        Boost::graph)

add_executable(
        ${PROJECT_NAME}
        ${PROJECT_SOURCE_DIR}/src/server.cpp
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        ${PROJECT_NAME}-dep
        git2::git2
)

