# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if(EXISTS "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp/polyhook2-populate-gitclone-lastrun.txt" AND EXISTS "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp/polyhook2-populate-gitinfo.txt" AND
  "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp/polyhook2-populate-gitclone-lastrun.txt" IS_NEWER_THAN "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp/polyhook2-populate-gitinfo.txt")
  message(STATUS
    "Avoiding repeated git clone, stamp file is up to date: "
    "'G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp/polyhook2-populate-gitclone-lastrun.txt'"
  )
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: 'G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-src'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "C:/Users/Nils/AppData/Local/.xmake/packages/g/git/2.20.0/65b10f9e7885425fbf2d2abaad2a61a9/share/MinGit/cmd/git.exe"
            clone --progress --config "advice.detachedHead=false" "https://github.com/stevemk14ebr/PolyHook_2_0.git" "polyhook2-src"
    WORKING_DIRECTORY "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps"
    RESULT_VARIABLE error_code
  )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once: ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/stevemk14ebr/PolyHook_2_0.git'")
endif()

execute_process(
  COMMAND "C:/Users/Nils/AppData/Local/.xmake/packages/g/git/2.20.0/65b10f9e7885425fbf2d2abaad2a61a9/share/MinGit/cmd/git.exe"
          checkout "fd2a88f09c8ae89440858fc52573656141013c7f" --
  WORKING_DIRECTORY "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'fd2a88f09c8ae89440858fc52573656141013c7f'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "C:/Users/Nils/AppData/Local/.xmake/packages/g/git/2.20.0/65b10f9e7885425fbf2d2abaad2a61a9/share/MinGit/cmd/git.exe" 
            submodule update --recursive --init 
    WORKING_DIRECTORY "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-src"
    RESULT_VARIABLE error_code
  )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: 'G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-src'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp/polyhook2-populate-gitinfo.txt" "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp/polyhook2-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: 'G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp/polyhook2-populate-gitclone-lastrun.txt'")
endif()
