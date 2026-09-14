cmake_minimum_required(VERSION 3.21)
project(SlimenanoVirtualFileSystem
  VERSION 1.0.0
  DESCRIPTION "This is a free virtual file system"
  HOMEPAGE_URL "https://github.com/Ren-Pen/slimenano-vfs"
  LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(NAMESPACE slimenano)
set(TARGET_NAME vfs)
set(TARGET ${NAMESPACE}_${TARGET_NAME})

add_library(${TARGET} STATIC
  "src/pch.cpp"
  "src/Path.cpp"
  "src/Virtual/VirtualFileSystem.cpp"
  "src/Native/NativeFileSystem.cpp"
  "src/Native/NativeFileHandle.cpp"
  "src/Native/NativeUtils.cpp"
  "src/Zip/ZipFileSystem.cpp"
  "src/Zip/ZipErrorCategory.cpp"
  "src/Zip/ZipFileHandle.cpp"
)

add_library(${NAMESPACE}::${TARGET_NAME} ALIAS ${TARGET})

target_include_directories(${TARGET}
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
  # /utf-8: treat source files and narrow string literals as UTF-8 instead of
  # the ANSI code page, matching the VFS path encoding contract.
  target_compile_options(${TARGET} PRIVATE /W4 /WX /utf-8)
else()
  target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_compile_features(${TARGET} PUBLIC cxx_std_20)

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

option(SLIMENANO_VFS_BUILD_DEMO "Build VFS demo" OFF)

if(SLIMENANO_VFS_BUILD_DEMO)
  add_subdirectory(demo)
endif()

# INSTALL CONFIG
set_target_properties(${TARGET} PROPERTIES
  EXPORT_NAME ${TARGET_NAME}
  VERSION   ${PROJECT_VERSION}
)

set(INSTALL_TARGET ${TARGET}Targets)
set(INSTALL_CONFIG ${TARGET}Config)
set(SLIMENANO_VFS_INSTALL_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET})

install(TARGETS ${TARGET}
  EXPORT ${INSTALL_TARGET}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT ${INSTALL_TARGET}
  FILE  ${INSTALL_TARGET}.cmake
  NAMESPACE ${NAMESPACE}::
  DESTINATION ${SLIMENANO_VFS_INSTALL_CMAKE_DIR}
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${INSTALL_CONFIG}.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CONFIG}.cmake
  INSTALL_DESTINATION ${SLIMENANO_VFS_INSTALL_CMAKE_DIR}
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CONFIG}Version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CONFIG}.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CONFIG}Version.cmake
  DESTINATION ${SLIMENANO_VFS_INSTALL_CMAKE_DIR}
)
