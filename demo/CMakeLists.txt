set(NAMESPACE slimenano)
set(TARGET_NAME vfs_demo)

set(TARGET ${NAMESPACE}_${TARGET_NAME})

add_executable(${TARGET}
    "src/main.cpp"
)

if(MSVC)
    # /utf-8: treat source files and narrow string literals as UTF-8 instead of
    # the ANSI code page, matching the VFS path encoding contract.
    target_compile_options(${TARGET} PRIVATE /W4 /WX /utf-8)
else()
    target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_compile_features(${TARGET} PRIVATE cxx_std_20)

target_link_libraries(${TARGET} PRIVATE slimenano::vfs)
