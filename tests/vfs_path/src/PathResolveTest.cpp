#include <gtest/gtest.h>
#include <slimenano/vfs/Path.h>

#include <string>

using slimenano::filesystem::Path;

// NOTE: The old tests called Path::ToAbsolute() with no argument
// (e.g. Path("foo/bar").ToAbsolute()). The current API is
// Path::ToAbsolute(const Path& base), so all cases below pass an explicit
// anchor. If a no-arg overload is reintroduced, add cases for it separately.

// ---------------------------------------------------------------------------
// ToAbsolute(base)
// ---------------------------------------------------------------------------

struct ToAbsoluteCase {
    std::string path;
    std::string base;
    std::string result;
};

class PathToAbsoluteTest : public ::testing::TestWithParam<ToAbsoluteCase> {};

TEST_P(PathToAbsoluteTest, ToAbsolute) {
    const Path p(GetParam().path);
    const Path base(GetParam().base);
    EXPECT_EQ(p.ToAbsolute(base).String(), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(
    ToAbsolute,
    PathToAbsoluteTest,
    ::testing::Values(
        // already absolute: base is ignored
        ToAbsoluteCase{"/foo", "/base", "/foo"},
        ToAbsoluteCase{"/foo/bar", "/base", "/foo/bar"},

        // relative joined onto absolute base
        ToAbsoluteCase{"x", "/base", "/base/x"},
        ToAbsoluteCase{"foo/bar", "/", "/foo/bar"},
        ToAbsoluteCase{"foo/bar", "/base", "/base/foo/bar"}

        // TODO: decide and pin down behavior when base itself is relative.
        // The current implementation prepends '/'; if that is the intended
        // contract, add explicit cases here. Otherwise consider asserting
        // base must be absolute.
    )
);

// ---------------------------------------------------------------------------
// ToRelative(base)
// ---------------------------------------------------------------------------

struct ToRelativeCase {
    std::string path;
    std::string base;
    std::string result;
};

class PathToRelativeTest : public ::testing::TestWithParam<ToRelativeCase> {};

TEST_P(PathToRelativeTest, ToRelative) {
    const Path p(GetParam().path);
    const Path base(GetParam().base);
    EXPECT_EQ(p.ToRelative(base).String(), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(
    ToRelative,
    PathToRelativeTest,
    ::testing::Values(
        // base is a prefix of path
        ToRelativeCase{"/a/b/c", "/a", "b/c"},
        ToRelativeCase{"/a", "/", "a"},

        // equal path and base yields empty
        ToRelativeCase{"/a/b", "/a/b", ""},

        // divergent paths produce dot-dot segments
        ToRelativeCase{"/a/c/d", "/a/b", "../c/d"},
        ToRelativeCase{"/foo/bar", "/c/d", "../../foo/bar"},

        // TODO: one-absolute-one-relative currently returns empty Path.
        // Confirm this is the desired contract.
        ToRelativeCase{"a/b", "/a", ""}
    )
);
