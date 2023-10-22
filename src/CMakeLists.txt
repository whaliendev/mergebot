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
# 定义一个列表，其中包含您要查找的所有字符串
set(EXCLUDE_STRINGS "ASTBasedHandler" "GraphMerger" "graph_export")

foreach (file ${RTTI_DISABLED_SRCS})
    set(EXCLUDE_FILE FALSE)

    # 检查文件名是否包含列表中的任何一个字符串
    foreach (exclude_string ${EXCLUDE_STRINGS})
        string(FIND ${file} ${exclude_string} position)
        if (NOT ${position} EQUAL -1)
            set(EXCLUDE_FILE TRUE)
            break()
        endif ()
    endforeach ()

    # 如果文件名不包含任何排除字符串，则设置编译标志
    if (NOT ${EXCLUDE_FILE})
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

