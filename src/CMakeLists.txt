find_package(fmt)

#find_package(Clang REQUIRED)

#if(Clang_FOUND)
#  message(STATUS "Found Clang ${Clang_PACKAGE_VERSION}")
#  message(STATUS "Clang found at ${Clang_DIR}")
#endif(Clang_FOUND)

#find_package(LLVM REQUIRED CONFIG)
#message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
#message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
#
#include_directories(${LLVM_INCLUDE_DIRS})
#separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
#add_definitions(${LLVM_DEFINITIONS_LIST})

add_executable(
        ${PROJECT_NAME}
        server.cpp
        result_vo_utils.cpp
        result_vo_utils.h
        controller/config_controller.cpp
        controller/config_controller.h
        controller/project_controller.cpp
        controller/project_controller.h
        controller/resolve_controller.cpp
        controller/resolve_controller.h
        core/MergeScenario.cpp
        core/MergeScenario.h
        ../include/base/String.h
        ../include/base/String.cpp
        ../include/base/List.h
        ../include/base/Hash.h
        ../include/base/LinkedList.h
        ../include/base/Map.h
        ../include/base/Set.h
        ../include/base/ThreadPool.cpp
        ../include/base/ThreadPool.h)

target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/include")

#llvm_map_components_to_libnames(llvm_libs
#        support
#        core
#        option
#        frontendopenmp
#        )

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        fmt::fmt
#        ${llvm_libs}
#        clang-cpp
)

