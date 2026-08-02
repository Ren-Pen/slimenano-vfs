cmake_minimum_required(VERSION 3.21)
project(SlimenanoVirtualFileSystem
    VERSION 0.0.1
    DESCRIPTION "This is a free virtual file system"
    HOMEPAGE_URL "https://github.com/Ren-Pen/slimenano-vfs"
    LANGUAGES CXX
)


set(NAMESPACE slimenano)
set(TARGET_NAME vfs)
set(TARGET ${NAMESPACE}_${TARGET_NAME})

add_library(${TARGET}
"src/pch.cpp"
"src/Path.cpp"
"src/VirtualFileSystem.cpp"
"src/NativeFileSystem.cpp"
"src/NativeFileHandle.cpp"
)

add_library(${NAMESPACE}::${TARGET_NAME} ALIAS ${TARGET})

target_include_directories(${TARGET}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(${TARGET} PUBLIC cxx_std_20)

if(MSVC)
  # /utf-8: treat source files and narrow string literals as UTF-8 instead of
  # the ANSI code page, matching the VFS path encoding contract.
  target_compile_options(${TARGET} PRIVATE /W4 /WX /utf-8)
else()
  target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_compile_features(${TARGET} PRIVATE cxx_std_20)

target_precompile_headers(${TARGET} PRIVATE "src/pch.h")

find_package(libzip CONFIG REQUIRED)
target_link_libraries(${TARGET} PRIVATE libzip::zip)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(SLIMENANO_VFS_BUILD_TESTS "Build VFS tests" ON)
else()
    option(SLIMENANO_VFS_BUILD_TESTS "Build VFS tests" OFF)
endif()

if(SLIMENANO_VFS_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()
