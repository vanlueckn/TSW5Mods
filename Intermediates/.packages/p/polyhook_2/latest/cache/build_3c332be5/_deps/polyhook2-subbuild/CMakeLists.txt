# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.29.5-msvc4)

# Reject any attempt to use a toolchain file. We must not use one because
# we could be downloading it here. If the CMAKE_TOOLCHAIN_FILE environment
# variable is set, the cache variable will have been initialized from it.
unset(CMAKE_TOOLCHAIN_FILE CACHE)
unset(ENV{CMAKE_TOOLCHAIN_FILE})

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(polyhook2-populate NONE)


# Pass through things we've already detected in the main project to avoid
# paying the cost of redetecting them again in ExternalProject_Add()
set(GIT_EXECUTABLE [==[C:/Users/Nils/AppData/Local/.xmake/packages/g/git/2.20.0/65b10f9e7885425fbf2d2abaad2a61a9/share/MinGit/cmd/git.exe]==])
set(GIT_VERSION_STRING [==[2.20.0.windows.1]==])
set_property(GLOBAL PROPERTY _CMAKE_FindGit_GIT_EXECUTABLE_VERSION
  [==[C:/Users/Nils/AppData/Local/.xmake/packages/g/git/2.20.0/65b10f9e7885425fbf2d2abaad2a61a9/share/MinGit/cmd/git.exe;2.20.0.windows.1]==]
)


include(ExternalProject)
ExternalProject_Add(polyhook2-populate
                     "UPDATE_DISCONNECTED" "False" "GIT_REPOSITORY" "https://github.com/stevemk14ebr/PolyHook_2_0.git" "EXTERNALPROJECT_INTERNAL_ARGUMENT_SEPARATOR" "GIT_TAG" "fd2a88f09c8ae89440858fc52573656141013c7f" "GIT_PROGRESS" "ON"
                    SOURCE_DIR          "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-src"
                    BINARY_DIR          "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


