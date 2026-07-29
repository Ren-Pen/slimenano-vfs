#include <gtest/gtest.h>
#include <slimenano/vfs/Path.h>

#include <string>

using slimenano::filesystem::Path;

// ---------------------------------------------------------------------------
// operator/ (join)
// ---------------------------------------------------------------------------

struct JoinCase {
    std::string lhs;
    std::string rhs;
    std::string result;
};

class PathJoinTest : public ::testing::TestWithParam<JoinCase> {};

TEST_P(PathJoinTest, Join) {
    const Path lhs(GetParam().lhs);
    const Path rhs(GetParam().rhs);
    EXPECT_EQ((lhs / rhs).String(), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(
    Join,
    PathJoinTest,
    ::testing::Values(
        JoinCase{"a", "b", "a/b"},
        JoinCase{"foo", "bar", "foo/bar"},
        JoinCase{"/a", "b", "/a/b"},
        JoinCase{"/foo", "bar", "/foo/bar"},
        JoinCase{"/", "b", "/b"},

        // empty operands
        JoinCase{"", "a", "a"},
        JoinCase{"", "bar", "bar"},
        JoinCase{"a", "", "a"},
        JoinCase{"foo", "", "foo"},

        // absolute rhs wins
        JoinCase{"foo", "/bar", "/bar"},

        // result is renormalized
        JoinCase{"a", "../b", "b"}
    )
);

// ---------------------------------------------------------------------------
// Equality
// ---------------------------------------------------------------------------

TEST(PathCompare, EqualAfterNormalization) {
    EXPECT_EQ(Path("foo/"), Path("foo"));
    EXPECT_EQ(Path("a//b"), Path("a/b"));
    EXPECT_EQ(Path("a\\b"), Path("a/b"));
    EXPECT_EQ(Path("./a"), Path("a"));
}

TEST(PathCompare, Inequality) {
    EXPECT_NE(Path("a"), Path("b"));
    EXPECT_NE(Path("/a"), Path("a"));
}

// ---------------------------------------------------------------------------
// Ordering (operator<=>)
// ---------------------------------------------------------------------------

TEST(PathCompare, Ordering) {
    EXPECT_LT(Path("a"), Path("b"));
    // absolute paths sort before relative ones because '/' < 'a' in byte order
    EXPECT_LT(Path("/a"), Path("a"));
}
