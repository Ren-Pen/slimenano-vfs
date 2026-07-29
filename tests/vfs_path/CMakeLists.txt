set(NAMESPACE slimenano)
set(TARGET_NAME vfs_path_test)

set(TARGET ${NAMESPACE}_${TARGET_NAME})

add_executable(${TARGET}
    "src/PathBaseTest.cpp"
    "src/PathIteratorTest.cpp"
    "src/PathJoinCompareTest.cpp"
    "src/PathNormalizeTest.cpp"
    "src/PathParentTest.cpp"
    "src/PathResolveTest.cpp"
)

target_link_libraries(${TARGET} PRIVATE GTest::gtest)
target_link_libraries(${TARGET} PRIVATE GTest::gtest_main)
target_link_libraries(${TARGET} PRIVATE slimenano::vfs)

include(GoogleTest)
gtest_discover_tests(${TARGET})
