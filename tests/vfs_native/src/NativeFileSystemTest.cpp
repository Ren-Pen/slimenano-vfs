#include <slimenano/vfs/NativeFileSystem.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace slimenano::filesystem {
namespace {

namespace fs = std::filesystem;

class NativeFileSystemEncodingTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::error_code ec;
        m_dir = fs::temp_directory_path() / "slimenano-vfs-encoding-test";
        fs::remove_all(m_dir, ec);
        fs::create_directories(m_dir, ec);
        ASSERT_FALSE(ec) << ec.message();

        const auto u8Root = m_dir.u8string();
        m_fs = std::make_unique<NativeFileSystem>(
            std::string(reinterpret_cast<const char*>(u8Root.data()), u8Root.size())
        );
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_dir, ec);
    }

    fs::path m_dir;
    std::unique_ptr<NativeFileSystem> m_fs;
};

TEST_F(NativeFileSystemEncodingTest, Utf8FileRoundTrip) {
    const std::string name = "中文文件.txt";
    const Path path{"/" + name};

    std::error_code ec;
    m_fs->CreateFile(path, ec);
    ASSERT_FALSE(ec) << ec.message();

    ASSERT_TRUE(m_fs->Exists(path, ec));
    ASSERT_FALSE(ec);

    const auto info = m_fs->Stat(path, ec);
    ASSERT_FALSE(ec);
    ASSERT_EQ(info.type, FileType::Regular);

    const auto names = m_fs->List(Path{"/"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_NE(std::find(names.begin(), names.end(), name), names.end());
}

TEST_F(NativeFileSystemEncodingTest, Utf8DirectoryRoundTrip) {
    const std::string dirName = "中文目录";

    std::error_code ec;
    m_fs->CreateDirectory(Path{"/" + dirName}, ec);
    ASSERT_FALSE(ec) << ec.message();

    ASSERT_TRUE(m_fs->IsDirectory(Path{"/" + dirName}, ec));
    ASSERT_FALSE(ec);

    const auto names = m_fs->List(Path{"/"}, ec);
    ASSERT_FALSE(ec);
    EXPECT_NE(std::find(names.begin(), names.end(), dirName), names.end());
}

TEST_F(NativeFileSystemEncodingTest, OpenWriteReadOnUtf8Path) {
    const std::string name = "打开读写.txt";
    const Path path{"/" + name};

    std::error_code ec;
    auto handle = m_fs->Open(path, OpenOption::Read | OpenOption::Create, ec);
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

} // namespace
} // namespace slimenano::filesystem
