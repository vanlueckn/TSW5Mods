# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-src"
  "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-build"
  "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix"
  "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/tmp"
  "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp"
  "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src"
  "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "G:/TSW5Mods/Intermediates/.packages/p/polyhook_2/latest/cache/build_3c332be5/_deps/polyhook2-subbuild/polyhook2-populate-prefix/src/polyhook2-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
