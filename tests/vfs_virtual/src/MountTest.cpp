#include <slimenano/vfs/vfs.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace slimenano::filesystem {
namespace {

namespace fs = std::filesystem;

class VirtualFileSystemMountTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::error_code ec;
        m_dir = fs::temp_directory_path() / "slimenano-vfs-virtual-mount-test";
        fs::remove_all(m_dir, ec);
        fs::create_directories(m_dir, ec);
        ASSERT_FALSE(ec) << ec.message();

        m_vfs = std::make_unique<VirtualFileSystem>();
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_dir, ec);
    }

    // Creates a NativeFileSystem rooted at a (possibly new) subdirectory of
    // the test directory.
    std::shared_ptr<NativeFileSystem> nativeRootedAt(const std::string& sub) const {
        std::error_code ec;
        fs::create_directories(m_dir / sub, ec);
        const auto u8Root = (m_dir / sub).u8string();
        return std::make_shared<NativeFileSystem>(
            std::string(reinterpret_cast<const char*>(u8Root.data()), u8Root.size())
        );
    }

    fs::path m_dir;
    std::unique_ptr<VirtualFileSystem> m_vfs;
};

TEST_F(VirtualFileSystemMountTest, MountThenUnmount) {
    const Path mountPoint{"/data"};
    auto fs = nativeRootedAt("data");

    m_vfs->Mount(mountPoint, fs);
    ASSERT_TRUE(m_vfs->IsMounted(mountPoint));
    ASSERT_TRUE(m_vfs->IsMounted(mountPoint, fs));

    m_vfs->Unmount(mountPoint);
    EXPECT_FALSE(m_vfs->IsMounted(mountPoint));
    EXPECT_FALSE(m_vfs->IsMounted(mountPoint, fs));
}

TEST_F(VirtualFileSystemMountTest, IsMountedOnUnknownPathIsFalse) {
    EXPECT_FALSE(m_vfs->IsMounted(Path{"/not-mounted"}));
}

TEST_F(VirtualFileSystemMountTest, MountNullPointerIsIgnored) {
    m_vfs->Mount(Path{"/x"}, nullptr);
    EXPECT_FALSE(m_vfs->IsMounted(Path{"/x"}));
}

TEST_F(VirtualFileSystemMountTest, MountingSameInstanceTwiceIsNoOp) {
    const Path mountPoint{"/dup"};
    auto fs = nativeRootedAt("dup");
    m_vfs->Mount(mountPoint, fs);
    m_vfs->Mount(mountPoint, fs);
    EXPECT_EQ(m_vfs->GetMountedFileSystems(mountPoint).size(), 1u);
}

TEST_F(VirtualFileSystemMountTest, GetMountedFileSystemsKeepsMountOrder) {
    const Path mountPoint{"/stack"};
    auto first = nativeRootedAt("stack");
    auto second = nativeRootedAt("stack");
    m_vfs->Mount(mountPoint, first);
    m_vfs->Mount(mountPoint, second);

    const auto mounted = m_vfs->GetMountedFileSystems(mountPoint);
    ASSERT_EQ(mounted.size(), 2u);
    EXPECT_EQ(mounted[0], first);
    EXPECT_EQ(mounted[1], second);
}

TEST_F(VirtualFileSystemMountTest, GetMountedFileSystemsOnUnknownPathIsEmpty) {
    EXPECT_TRUE(m_vfs->GetMountedFileSystems(Path{"/nothing"}).empty());
}

TEST_F(VirtualFileSystemMountTest, UnmountSpecificInstance) {
    const Path mountPoint{"/two"};
    auto first = nativeRootedAt("two");
    auto second = nativeRootedAt("two");
    m_vfs->Mount(mountPoint, first);
    m_vfs->Mount(mountPoint, second);

    m_vfs->Unmount(mountPoint, first);
    EXPECT_FALSE(m_vfs->IsMounted(mountPoint, first));
    EXPECT_TRUE(m_vfs->IsMounted(mountPoint, second));
    ASSERT_EQ(m_vfs->GetMountedFileSystems(mountPoint).size(), 1u);
}

TEST_F(VirtualFileSystemMountTest, MountCreatesIntermediateVirtualDirectories) {
    const Path mountPoint{"/a/b/c"};
    m_vfs->Mount(mountPoint, nativeRootedAt("a"));

    EXPECT_TRUE(m_vfs->IsMounted(mountPoint));
    EXPECT_FALSE(m_vfs->IsMounted(Path{"/a"}));

    std::error_code ec;
    EXPECT_TRUE(m_vfs->Exists(Path{"/a"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_TRUE(m_vfs->IsDirectory(Path{"/a"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_TRUE(m_vfs->IsDirectory(Path{"/a/b"}, ec));
    ASSERT_FALSE(ec);
}

TEST_F(VirtualFileSystemMountTest, VirtualDirectoriesReportReadOnlyDirectoryInfo) {
    m_vfs->Mount(Path{"/a/b"}, nativeRootedAt("a"));

    std::error_code ec;
    EXPECT_TRUE(m_vfs->IsReadable(Path{"/a"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_FALSE(m_vfs->IsWritable(Path{"/a"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_EQ(m_vfs->Size(Path{"/a"}, ec), 0u);
    ASSERT_FALSE(ec);

    const auto info = m_vfs->Stat(Path{"/a"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(info.type, FileType::Directory);
    EXPECT_TRUE(info.readable);
    EXPECT_FALSE(info.writable);
}

TEST_F(VirtualFileSystemMountTest, EmptyVirtualFileSystemExposesReadOnlyRoot) {
    std::error_code ec;
    EXPECT_TRUE(m_vfs->Exists(Path{"/"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_TRUE(m_vfs->IsDirectory(Path{"/"}, ec));
    ASSERT_FALSE(ec);

    auto handle = m_vfs->Open(Path{"/"}, OpenOption::Create, ec);
    EXPECT_EQ(handle, nullptr);
    EXPECT_EQ(ec, std::make_error_code(std::errc::read_only_file_system));
}

TEST_F(VirtualFileSystemMountTest, UnmountPrunesEmptyVirtualDirectories) {
    const Path mountPoint{"/a/b"};
    m_vfs->Mount(mountPoint, nativeRootedAt("a"));

    std::error_code ec;
    auto names = m_vfs->List(Path{"/"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(names, (std::vector<std::string>{"a"}));

    m_vfs->Unmount(mountPoint);
    names = m_vfs->List(Path{"/"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_TRUE(names.empty());
}

TEST_F(VirtualFileSystemMountTest, CreateAndMountSucceeds) {
    const auto u8Root = m_dir.u8string();
    const std::string root(reinterpret_cast<const char*>(u8Root.data()), u8Root.size());

    std::error_code ec;
    auto fs = m_vfs->CreateAndMount<NativeFileSystem>("/", ec, root);
    ASSERT_NE(fs, nullptr);
    ASSERT_FALSE(ec) << ec.message();
    EXPECT_TRUE(m_vfs->IsMounted(Path{"/"}, fs));
    EXPECT_EQ(m_vfs->GetMountedFileSystems(Path{"/"}).size(), 1u);
}

TEST_F(VirtualFileSystemMountTest, CreateAndMountFailsWhenInitializeFails) {
    const auto u8Root = (m_dir / "does-not-exist").u8string();
    const std::string missingRoot(reinterpret_cast<const char*>(u8Root.data()), u8Root.size());

    std::error_code ec;
    auto fs = m_vfs->CreateAndMount<NativeFileSystem>("/", ec, missingRoot);
    EXPECT_EQ(fs, nullptr);
    // The exact code is platform-dependent (no_such_file_or_directory vs
    // not_a_directory); the contract is that the failure is reported.
    EXPECT_TRUE(static_cast<bool>(ec)) << "Initialize failure must be reported";
    EXPECT_FALSE(m_vfs->IsMounted(Path{"/"}));
}

} // namespace
} // namespace slimenano::filesystem
