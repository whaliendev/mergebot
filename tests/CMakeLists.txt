find_package(Clang REQUIRED)
find_package(re2 REQUIRED)

if (Clang_FOUND)
    message(STATUS " Found Clang ${Clang_PACKAGE_VERSION} ")
    message(STATUS " Clang found at ${Clang_DIR} ")
endif (Clang_FOUND)

find_package(LLVM REQUIRED CONFIG)
message(STATUS " Found LLVM ${LLVM_PACKAGE_VERSION} ")
message(STATUS " Using LLVMConfig.cmake in: ${LLVM_DIR} ")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs support)

find_package(fmt REQUIRED)

# copy mock data files to output binary dir
file(GLOB_RECURSE MOCK_DATA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/mock/*")
foreach (MOCK_DATA_FILE ${MOCK_DATA_FILES})
    get_filename_component(DATA_FILE_NAME ${MOCK_DATA_FILE} NAME)
    configure_file(
            ${MOCK_DATA_FILE}
            ${MB_OUTPUT_DIR}/mock/${DATA_FILE_NAME}
            COPYONLY
    )
endforeach ()

# build all tests
file(GLOB_RECURSE TEST_SRC_FILES
        CONFIGURE_DEPENDS ./*test*.cpp ./*Test*.cpp main.cpp)

add_executable(runUnitTests ${TEST_SRC_FILES})

target_link_libraries(runUnitTests gtest)

target_link_libraries(runUnitTests
        ${PROJECT_NAME}_lib
        ${llvm_libs}
        clang-cpp
        fmt::fmt-header-only
        re2::re2)

add_test(UnitTests runUnitTests)
