find_package(fmt)

find_package(Clang REQUIRED)

if(Clang_FOUND)
    message(STATUS "Found Clang ${Clang_PACKAGE_VERSION}")
    message(STATUS "Clang found at ${Clang_DIR}")
endif(Clang_FOUND)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_executable(
    ${PROJECT_NAME}
        app.cpp
        clp/command_line_parser.h
        mergebot.cpp
        mergebot.h
        ../include/utils/String.h
        ../include/utils/List.h
        ../include/utils/String.cpp
        ../include/utils/Hash.h
        ../include/utils/LinkedList.h
        ../include/utils/Map.h ../include/utils/Set.h)
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/include")

llvm_map_components_to_libnames(llvm_libs
    support
    core
    option
    frontendopenmp
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    fmt::fmt
    ${llvm_libs}
    clang-cpp
)

