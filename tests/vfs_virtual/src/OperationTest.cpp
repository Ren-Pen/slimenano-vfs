#include <slimenano/vfs/vfs.h>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace slimenano::filesystem {
namespace {

namespace fs = std::filesystem;

/**
 * Fixture with a NativeFileSystem mounted at "/" so every virtual path is
 * backed by a real directory under the test directory.
 */
class VirtualFileSystemOperationTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::error_code ec;
        m_dir = fs::temp_directory_path() / "slimenano-vfs-virtual-operation-test";
        fs::remove_all(m_dir, ec);
        fs::create_directories(m_dir, ec);
        ASSERT_FALSE(ec) << ec.message();

        m_vfs = std::make_unique<VirtualFileSystem>();
        m_vfs->Mount(Path{"/"}, nativeRootedAt(""));
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

TEST_F(VirtualFileSystemOperationTest, ExistsDelegatesToMounts) {
    std::error_code ec;
    m_vfs->CreateFile(Path{"/a.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();

    EXPECT_TRUE(m_vfs->Exists(Path{"/a.txt"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_FALSE(m_vfs->Exists(Path{"/missing.txt"}, ec));
    ASSERT_FALSE(ec);
}

TEST_F(VirtualFileSystemOperationTest, StatRoundTrip) {
    std::error_code ec;
    m_vfs->CreateFile(Path{"/a.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();

    const auto info = m_vfs->Stat(Path{"/a.txt"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(info.type, FileType::Regular);
    EXPECT_EQ(info.size, 0u);
    EXPECT_TRUE(info.Exists());

    const auto missing = m_vfs->Stat(Path{"/missing.txt"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(missing.type, FileType::None);
    EXPECT_FALSE(missing.Exists());
}

TEST_F(VirtualFileSystemOperationTest, CreateDirectoriesAndQuery) {
    std::error_code ec;
    m_vfs->CreateDirectories(Path{"/a/b/c"}, ec);
    ASSERT_FALSE(ec) << ec.message();

    EXPECT_TRUE(m_vfs->IsDirectory(Path{"/a/b/c"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_FALSE(m_vfs->IsRegularFile(Path{"/a/b/c"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_TRUE(m_vfs->IsReadable(Path{"/a/b/c"}, ec));
    ASSERT_FALSE(ec);
}

TEST_F(VirtualFileSystemOperationTest, DeleteFileAndDirectories) {
    std::error_code ec;
    m_vfs->CreateDirectories(Path{"/d/e"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    m_vfs->CreateFile(Path{"/d/e/f.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();

    m_vfs->DeleteFile(Path{"/d/e/f.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    EXPECT_FALSE(m_vfs->Exists(Path{"/d/e/f.txt"}, ec));
    ASSERT_FALSE(ec);

    m_vfs->DeleteDirectories(Path{"/d"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    EXPECT_FALSE(m_vfs->Exists(Path{"/d"}, ec));
    ASSERT_FALSE(ec);
}

TEST_F(VirtualFileSystemOperationTest, OpenWriteSeekReadRoundTrip) {
    std::error_code ec;
    auto handle = m_vfs->Open(Path{"/file.bin"}, OpenOption::Read | OpenOption::Create, ec);
    ASSERT_NE(handle, nullptr);
    ASSERT_FALSE(ec) << ec.message();

    const std::array<std::byte, 5> payload{
        std::byte{0x31}, std::byte{0x32}, std::byte{0x33}, std::byte{0x34}, std::byte{0x35}
    };
    ASSERT_EQ(handle->Write(payload, ec), payload.size());
    ASSERT_FALSE(ec);
    ASSERT_EQ(handle->Seek(0, SeekOrigin::Begin, ec), 0u);
    ASSERT_FALSE(ec);

    std::array<std::byte, 5> buffer{};
    ASSERT_EQ(handle->Read(buffer, ec), buffer.size());
    ASSERT_FALSE(ec);
    EXPECT_EQ(buffer, payload);

    handle->Close(ec);
    ASSERT_FALSE(ec);
}

TEST_F(VirtualFileSystemOperationTest, OpenConvenienceOverloadReads) {
    std::error_code ec;
    m_vfs->CreateFile(Path{"/a.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();

    auto handle = m_vfs->Open(Path{"/a.txt"}, ec);
    ASSERT_NE(handle, nullptr);
    ASSERT_FALSE(ec);
}

TEST_F(VirtualFileSystemOperationTest, OpenReadOnlyOnMissingEntryFails) {
    std::error_code ec;
    auto handle = m_vfs->Open(Path{"/missing.txt"}, OpenOption::Read, ec);
    EXPECT_EQ(handle, nullptr);
    EXPECT_EQ(ec, std::make_error_code(std::errc::no_such_file_or_directory));
}

TEST_F(VirtualFileSystemOperationTest, Utf8PathRoundTrip) {
    std::error_code ec;
    m_vfs->CreateDirectories(Path{"/中文目录"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    m_vfs->CreateFile(Path{"/中文目录/文件.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();

    EXPECT_TRUE(m_vfs->Exists(Path{"/中文目录/文件.txt"}, ec));
    ASSERT_FALSE(ec);

    const auto names = m_vfs->List(Path{"/中文目录"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(names, (std::vector<std::string>{"文件.txt"}));
}

TEST_F(VirtualFileSystemOperationTest, ListMergesMountContentsAndVirtualChildren) {
    // Second native file system mounted at /data/backup with its own content;
    // "backup" itself exists only as a virtual directory node.
    auto dataFs = nativeRootedAt("data");
    std::error_code ec;
    dataFs->CreateFile(Path{"/inner.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    m_vfs->Mount(Path{"/data/backup"}, dataFs);

    m_vfs->CreateFile(Path{"/root.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();

    const auto dataNames = m_vfs->List(Path{"/data"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(dataNames, (std::vector<std::string>{"backup", "inner.txt"}));
}

TEST_F(VirtualFileSystemOperationTest, DeepestAncestorMountWins) {
    auto dataFs = nativeRootedAt("data");
    std::error_code ec;
    dataFs->CreateFile(Path{"/inner.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    m_vfs->Mount(Path{"/data"}, dataFs);

    // The file only exists inside the /data mount, not in the root mount.
    EXPECT_TRUE(m_vfs->Exists(Path{"/data/inner.txt"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_FALSE(m_vfs->Exists(Path{"/inner.txt"}, ec));
    ASSERT_FALSE(ec);

    const auto info = m_vfs->Stat(Path{"/data/inner.txt"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(info.type, FileType::Regular);
}

TEST_F(VirtualFileSystemOperationTest, LastMountedWinsForConflictingEntries) {
    auto first = nativeRootedAt("fs1");
    auto second = nativeRootedAt("fs2");
    const Path mountPoint{"/shared"};

    std::error_code ec;
    first->CreateFile(Path{"/dup.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    second->CreateFile(Path{"/dup.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();

    // Give the two files different sizes so the winner is observable.
    auto handle = second->Open(Path{"/dup.txt"}, OpenOption::Append, ec);
    ASSERT_NE(handle, nullptr);
    ASSERT_FALSE(ec);
    const std::array<std::byte, 3> payload{std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
    ASSERT_EQ(handle->Write(payload, ec), payload.size());
    ASSERT_FALSE(ec);
    handle->Close(ec);
    ASSERT_FALSE(ec);

    m_vfs->Mount(mountPoint, first);
    m_vfs->Mount(mountPoint, second);

    // The most recently mounted file system wins for the conflicting entry.
    const auto size = m_vfs->Size(Path{"/shared/dup.txt"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(size, payload.size());
}

TEST_F(VirtualFileSystemOperationTest, UnmountedRootBecomesReadOnlyVirtualDirectory) {
    std::error_code ec;
    m_vfs->Unmount(Path{"/"});

    // The root still exists as a virtual directory: readable, not writable.
    EXPECT_TRUE(m_vfs->Exists(Path{"/"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_TRUE(m_vfs->IsDirectory(Path{"/"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_TRUE(m_vfs->IsReadable(Path{"/"}, ec));
    ASSERT_FALSE(ec);
    EXPECT_FALSE(m_vfs->IsWritable(Path{"/"}, ec));
    ASSERT_FALSE(ec);

    const auto info = m_vfs->Stat(Path{"/"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(info.type, FileType::Directory);
    EXPECT_TRUE(info.readable);
    EXPECT_FALSE(info.writable);
    EXPECT_EQ(info.size, 0u);

    // ...but opening it for any access fails as read-only.
    auto writeHandle = m_vfs->Open(Path{"/"}, OpenOption::Create, ec);
    EXPECT_EQ(writeHandle, nullptr);
    EXPECT_EQ(ec, std::make_error_code(std::errc::read_only_file_system));

    auto readHandle = m_vfs->Open(Path{"/"}, OpenOption::Read, ec);
    EXPECT_EQ(readHandle, nullptr);
    EXPECT_EQ(ec, std::make_error_code(std::errc::read_only_file_system));
}

} // namespace
} // namespace slimenano::filesystem
