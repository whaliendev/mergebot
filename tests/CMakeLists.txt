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

target_link_libraries(runUnitTests gtest gmock)

target_link_libraries(runUnitTests
        ${PROJECT_NAME}-dep)

add_test(UnitTests runUnitTests)
