find_package(Crow REQUIRED)
find_package(re2 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(magic_enum REQUIRED)
find_package(TBB REQUIRED)
find_package(Boost 1.71 REQUIRED
        COMPONENTS
        graph)
find_package(tree-sitter REQUIRED)
find_package(tree-sitter-cpp REQUIRED)
find_package(libgit2 REQUIRED)
find_package(zstd REQUIRED)

find_package(LLVM REQUIRED)
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ./*.cpp ./*.hpp)
list(FILTER SRC_FILES EXCLUDE REGEX "core/.*")
# if llvm compiles with -fno-rtti, all dependent srcs should compile with -fno-rtti
file(GLOB_RECURSE RTTI_DISABLED_SRCS CONFIGURE_DEPENDS ./core/*.cpp ./core/*.hpp)
foreach (file ${RTTI_DISABLED_SRCS})
    string(FIND ${file} "ASTBasedHandler" position)
    if (${position} EQUAL -1)
        set_source_files_properties(${file} PROPERTIES COMPILE_FLAGS "-fno-rtti")
    endif ()
endforeach ()

# extract src files for test purpose
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/server.cpp)

add_library(${PROJECT_NAME}-dep SHARED ${SRC_FILES} ${RTTI_DISABLED_SRCS})
target_link_libraries(${PROJECT_NAME}-dep
        #        clangBasic
        #        clangFormat
        #        clangFrontend
        #        clangLex
        #        clangTooling
        PUBLIC LLVMSupport
        PUBLIC fmt::fmt
        PUBLIC tree-sitter::tree-sitter
        PUBLIC tree-sitter-cpp::tree-sitter-cpp
        PUBLIC Crow::Crow
        PUBLIC spdlog::spdlog
        PRIVATE
        clang-cpp
        zstd::libzstd_static
        libgit2::libgit2
        magic_enum::magic_enum
        re2::re2
        TBB::tbb
        Boost::graph
        asio::asio
)

target_include_directories(${PROJECT_NAME}-dep
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${zstd_INCLUDE_DIRS}
        ${fmt_INCLUDE_DIRS}
        ${tree-sitter_INCLUDE_DIRS}
        ${tree-sitter-cpp_INCLUDE_DIRS}
        ${Crow_INCLUDE_DIRS}
        ${spdlog_INCLUDE_DIRS}
        ${nlohmann_json_INCLUDE_DIRS}
        ${re2_INCLUDE_DIRS}
        ${libgit2_INCLUDE_DIRS}
        ${magic_enum_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${asio_INCLUDE_DIRS}
)

add_executable(
        ${PROJECT_NAME}
        ${PROJECT_SOURCE_DIR}/src/server.cpp
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        ${PROJECT_NAME}-dep
)

