# check python version and conan
# if the check succeed, we install the dependencies in conanfile

find_program(
        CONAN_PROGRAM
        conan
        DOC "Use conan to install 3rd-party dependencies"
)

if (CONAN_PROGRAM)
    message(STATUS "Conan found in your system path")
    ### TODO(hwa): in release phase, we cannot limit conan profile to clang
    execute_process(
            COMMAND ${CONAN_PROGRAM} install . --output-folder ${PROJECT_BINARY_DIR}
            -b missing
            -r conancenter
            --profile:build clang
            --profile:host default
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE CONAN_INSTALL_RESULT
    )
    if (NOT CONAN_INSTALL_RESULT EQUAL "0")
        message(FATAL_ERROR "Conan fail to install the dependencies")
    else (NOT CONAN_INSTALL_RESULT EQUAL "0")
        message(STATUS "Conan succeed to install the dependencies")
    endif (NOT CONAN_INSTALL_RESULT EQUAL "0")
else (CONAN_PROGRAM)
    find_package(
            Python3
            REQUIRED
            COMPONENTS Interpreter
    )
    if (Python3_FOUND)
        if (Python3_VERSION VERSION_GREATER "3.6")
            message(FATAL_ERROR "We use conan to manage 3rd-party dependencies. \
            you can install conan by executing `pip install conan`")
        else (Python3_VERSION VERSION_GREATER "3.6")
            message(FATAL_ERROR "We use conan to manage 3rd-party dependencies. \
            While the python3 on your system is less than 3.6, you should upgrade\
            your python version to at least 3.6 to install conan by executing \
            `pip install conan`")
        endif (Python3_VERSION VERSION_GREATER "3.6")
    else (Python3_FOUND)
        message(FATAL_ERROR "We use conan to manage 3rd-party dependencies.\
        However, there seems no python installed on your system. \
        You should install at least a python3.6 and executing `pip install \
        conan` to install conan")
    endif (Python3_FOUND)
endif (CONAN_PROGRAM)
