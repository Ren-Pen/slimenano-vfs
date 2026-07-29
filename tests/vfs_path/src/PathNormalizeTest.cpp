#include <gtest/gtest.h>
#include <slimenano/vfs/Path.h>

using slimenano::filesystem::Path;

struct NormalizeCase {
    std::string input;
    std::string expected;
};

class PathNormalizeTest : public ::testing::TestWithParam<NormalizeCase> {};

TEST_P(PathNormalizeTest, Normalize) {
    EXPECT_EQ(Path(GetParam().input).String(), GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(
    Normalized,
    PathNormalizeTest,
    ::testing::Values(
        // empty and dot-only
        NormalizeCase{"", ""},
        NormalizeCase{".", ""},
        NormalizeCase{"./", ""},

        // trailing slash stripped
        NormalizeCase{"a/", "a"},
        NormalizeCase{"a//", "a"},
        NormalizeCase{"/a/", "/a"},

        // repeated slashes collapsed
        NormalizeCase{"a//b///c", "a/b/c"},
        NormalizeCase{"/a//b/", "/a/b"},

        // single dot dropped
        NormalizeCase{"./a", "a"},
        NormalizeCase{"a/./b", "a/b"},
        NormalizeCase{"./a/./b/.", "a/b"},

        // dot-dot resolved
        NormalizeCase{"a/../b", "b"},
        NormalizeCase{"a/b/../c", "a/c"},
        NormalizeCase{"a/b/../../c", "c"},

        // leading dot-dot kept when relative
        NormalizeCase{"../a", "../a"},
        NormalizeCase{"../../a", "../../a"},
        NormalizeCase{"a/../../..", "../.."},

        // leading dot-dot dropped when absolute
        NormalizeCase{"/../a", "/a"},
        NormalizeCase{"/../../a", "/a"},

        // collapse to root / empty
        NormalizeCase{"/a/b/../..", "/"},
        NormalizeCase{"a/b/../..", ""},

        // plain relative / absolute
        NormalizeCase{"a", "a"},
        NormalizeCase{"a/b", "a/b"},
        NormalizeCase{"/", "/"},
        NormalizeCase{"/a", "/a"},

        // backslash treated as separator
        NormalizeCase{"a\\b\\c", "a/b/c"},
        NormalizeCase{"a/b\\c", "a/b/c"},
        NormalizeCase{"\\a\\b\\c", "/a/b/c"},
        NormalizeCase{"a\\\\b////c", "a/b/c"}
    )
);
