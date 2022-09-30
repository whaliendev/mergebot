# check the installation of clang-format and clang-tidy 
# Notice that, for a non-developer, clang-format and clang-tidy aren't neccessary dependencies. 

find_program(
    CLANG_FORMAT_PROGRAM
    clang-format
    DOC "Use clang-format to unify thie coding style"  
)

find_program(
    CLANG_TIDY_PROGRAM
    clang-tidy
    DOC "Use clang-tidy to lint this project"
)

if(NOT CLANG_FORMAT_PROGRAM OR NOT CLANG_TIDY_PROGRAM)
    message(WARNING "We use clang-format and clang-tidy to do the lint work of\
    this project. So if you want to contribute to this project, consider installing\
    them first. Or you will fail at the pre-commit stage of git when submitting commits. ")
else(NOT CLANG_FORMAT_PROGRAM OR NOT CLANG_TIDY_PROGRAM)
    message(STATUS "Found clang-format and clang-tidy.")
endif(NOT CLANG_FORMAT_PROGRAM OR NOT CLANG_TIDY_PROGRAM)

