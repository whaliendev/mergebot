# we don't use conan actually
find_package(Crow REQUIRED)
find_package(re2 REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(magic_enum REQUIRED)
find_package(Clang REQUIRED)

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
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/server.cpp)
add_library(${PROJECT_NAME}_lib ${SRC_FILES})

add_executable(
        ${PROJECT_NAME}
        ${PROJECT_SOURCE_DIR}/src/server.cpp
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        clang-cpp
        ${llvm_libs}
        fmt::fmt-header-only
        magic_enum::magic_enum
        ${PROJECT_NAME}_lib
        re2::re2
)

