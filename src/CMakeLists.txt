find_package(LLVM REQUIRED CONFIG)

if(NOT LLVM_FOUND)
    message(FATALERROR "We need LLVM `LibTooling` lib to generate AST")
endif(NOT LLVM_FOUND)

#add_library(rmcx
#        ../include/mb/c.h
#        ../include/mb/export.h
#        )
#target_include_directories(rmcx
#        PRIVATE
#        "${PROJECT_SOURCE_DIR}/include")
