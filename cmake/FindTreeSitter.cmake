# Find TreeSitter library.

# Define flags for find_path/find_library commands.
set(TREE_SITTER_LIB_NAME tree-sitter)
set(TREE_SITTER_CPP_LIB_NAME tree-sitter-cpp)
set(TREE_SITTER_FIND_QUIETLY ${QUIETLY})
set(TREE_SITTER_FIND_NO_PACKAGE TRUE)

# Find TreeSitter library.
find_path(TREE_SITTER_INCLUDE_DIR NAMES tree_sitter/api.h)
find_library(TREE_SITTER_LIBRARIES NAMES ${TREE_SITTER_LIB_NAME})
find_library(TREE_SITTER_CPP_LIBRARIES NAMES ${TREE_SITTER_CPP_LIB_NAME})

if (NOT TREE_SITTER_INCLUDE_DIR OR NOT TREE_SITTER_LIBRARIES OR NOT TREE_SITTER_CPP_LIBRARIES)
    if (TREE_SITTER_FIND_QUIETLY)
        message(STATUS "Could not find TreeSitter library")
        return()
    else ()
        message(FATAL_ERROR "Could not find TreeSitter library")
    endif ()
endif ()

# Set the found libraries and includes of TreeSitter library.
set(TREE_SITTER_FOUND TRUE)
set(TREE_SITTER_INCLUDE_DIRS ${TREE_SITTER_INCLUDE_DIR})
set(TREE_SITTER_INCLUDE_DIR ${TREE_SITTER_INCLUDE_DIR} CACHE PATH "TreeSitter include directory")
set(TREE_SITTER_LIBRARIES ${TREE_SITTER_LIBRARIES} CACHE FILEPATH "TreeSitter libraries")
set(TREE_SITTER_CPP_LIBRARIES ${TREE_SITTER_CPP_LIBRARIES} CACHE FILEPATH "TreeSitter C++ libraries")

# Create an imported target for TreeSitter library.
add_library(tree-sitter::${TREE_SITTER_LIB_NAME} INTERFACE IMPORTED)
set_target_properties(tree-sitter::${TREE_SITTER_LIB_NAME} PROPERTIES
        IMPORTED_LOCATION ${TREE_SITTER_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${TREE_SITTER_INCLUDE_DIRS}
        )

# Create an imported target for TreeSitter C++ library.
add_library(tree-sitter::${TREE_SITTER_CPP_LIB_NAME} INTERFACE IMPORTED)
set_target_properties(tree-sitter::${TREE_SITTER_CPP_LIB_NAME} PROPERTIES
        IMPORTED_LOCATION ${TREE_SITTER_CPP_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${TREE_SITTER_INCLUDE_DIRS}
        )

# Add TreeSitter namespace alias for the targets.
#add_library(tree-sitter::tree-sitter ALIAS tree-sitter::${TREE_SITTER_LIB_NAME})
#add_library(tree-sitter::tree-sitter-cpp ALIAS tree-sitter::${TREE_SITTER_CPP_LIB_NAME})