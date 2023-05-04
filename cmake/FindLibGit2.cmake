# define
set(GIT2_LIB_NAME git2)
set(GIT2_FIND_QUIETLY ${QUIETLY})

# find libgit2 library
find_path(GIT2_INCLUDE_DIR NAMES git2/tree.h)

find_library(GIT2_LIBRARIES NAMES ${GIT2_LIB_NAME}
        PATHS /usr/lib /usr/local/lib /usr/lib64
        NO_DEFAULT_PATH)

if (NOT GIT2_INCLUDE_DIR OR NOT GIT2_LIBRARIES)
    if (GIT2_FIND_QUIETLY)
        message(STATUS "Could not find libgit2 library")
        return()
    else ()
        message(FATAL_ERROR "Could not find libgit2 library")
    endif ()
endif ()

# Set the found libraries and includes of LibGit2 library.
set(GIT2_FOUND TRUE)
set(GIT2_INCLUDE_DIRS ${GIT2_INCLUDE_DIR})
set(GIT2_INCLUDE_DIR ${GIT2_INCLUDE_DIR} CACHE PATH "LibGit2 include directory")
set(GIT2_LIBRARIES ${GIT2_LIBRARIES} CACHE FILEPATH "LibGit2 libraries")

# Create an imported target for LibGit2 library.
add_library(git2::${GIT2_LIB_NAME} UNKNOWN IMPORTED)
set_target_properties(git2::${GIT2_LIB_NAME}
        PROPERTIES IMPORTED_LOCATION ${GIT2_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${GIT2_INCLUDE_DIRS})

# Alias target for the imported target
#add_library(${GIT2_LIB_NAME}::${GIT2_LIB_NAME} ALIAS git2::${GIT2_LIB_NAME})