# Note Crow is different, Crow on conan is outdated
find_package(Crow REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

find_package(Clang REQUIRED)

if (Clang_FOUND)
  message(STATUS "Found Clang ${Clang_PACKAGE_VERSION}")
  message(STATUS "Clang found at ${Clang_DIR}")
endif (Clang_FOUND)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs support)

file(GLOB_RECURSE source CONFIGURE_DEPENDS ./*.h ./*.cpp ./*.hpp)

add_executable(
        ${PROJECT_NAME}
        ${source}
)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        fmt::fmt
        ${llvm_libs}
)

