set(NAMESPACE slimenano)
set(TARGET_NAME vfs_zip_test)

set(TARGET ${NAMESPACE}_${TARGET_NAME})

add_executable(${TARGET}
    "src/ZipFileSystemTest.cpp"
)

if(MSVC)
  # Test sources contain non-ASCII literals (u8"中文..."); make sure they are
  # read as UTF-8 rather than the ANSI code page.
  target_compile_options(${TARGET} PRIVATE /utf-8)
endif()

find_package(libzip CONFIG REQUIRED)
target_link_libraries(${TARGET} PRIVATE libzip::zip)

target_link_libraries(${TARGET} PRIVATE GTest::gtest)
target_link_libraries(${TARGET} PRIVATE GTest::gtest_main)
target_link_libraries(${TARGET} PRIVATE slimenano::vfs)

include(GoogleTest)
gtest_discover_tests(${TARGET})