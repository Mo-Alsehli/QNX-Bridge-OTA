#include <sys/stat.h>
#include <sys/types.h>

#include <CommonAPI/CommonAPI.hpp>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <v0/filetransfer/example/FileTransferStubDefault.hpp>
#include <vector>

namespace ft = v0::filetransfer::example;

static const std::string kUpdateDir = "data/server/";
static const std::string kUpdateImage = kUpdateDir + "rootfs.ext4";
static const std::string kUpdateVersion = kUpdateDir + "update.version";
static const std::string kUpdateCrc = kUpdateDir + "update.crc";

static const size_t CHUNK_SIZE = 64 * 1024;  // 64 KB

// Simple file-exists helper (C++14 compatible)
bool fileExists(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

// Get file size helper
bool getFileSize(const std::string& path, uint64_t& sizeOut) {
    struct stat sb;
    if (stat(path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
        sizeOut = static_cast<uint64_t>(sb.st_size);
        return true;
    }
    return false;
}

// Read uint32 from file helper
bool readUint32FromFile(const std::string& path, uint32_t& valueOut) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    std::string s;
    std::getline(in, s);
    if (s.empty()) return false;

    std::stringstream ss(s);
    if (s.find("0x") == 0 || s.find("0X") == 0)
        ss >> std::hex >> valueOut;
    else
        ss >> std::dec >> valueOut;

    return !ss.fail();
}

class FileTransferService : public ft::FileTransferStubDefault {
   public:
    FileTransferService() = default;

    void requestUpdate(const std::shared_ptr<CommonAPI::ClientId> /*_client*/, uint32_t currentVersion,
                       requestUpdateReply_t reply) override {
        ft::FileTransfer::UpdateInfo info;

        uint64_t fileSize = 0;
        uint32_t newVersion = 0;
        uint32_t crc = 0;

        // Default response
        info.setExists(false);
        info.setIsNew(false);
        info.setNewVersion(0);
        info.setSize(0);
        info.setCrc(0);
        info.setResultCode(-1);

        // Check image existence
        if (!fileExists(kUpdateImage)) {
            info.setResultCode(-10);
            reply(info);
            return;
        }

        info.setExists(true);

        // File size
        if (!getFileSize(kUpdateImage, fileSize)) {
            info.setResultCode(-11);
            reply(info);
            return;
        }
        info.setSize(fileSize);

        // Version file
        if (!readUint32FromFile(kUpdateVersion, newVersion)) {
            info.setResultCode(-12);
            reply(info);
            return;
        }
        info.setNewVersion(newVersion);

        // Optional CRC
        readUint32FromFile(kUpdateCrc, crc);
        info.setCrc(crc);

        // Version comparison
        info.setIsNew(newVersion > currentVersion);
        info.setResultCode(0);

        std::cout << "[Service] requestUpdate(): clientVersion=" << currentVersion << " newVersion=" << newVersion << std::endl;

        reply(info);
    }

    void startTransfer(const std::shared_ptr<CommonAPI::ClientId> /*_client*/, std::string /*_fileName*/,
                       startTransferReply_t reply) override {
        if (!fileExists(kUpdateImage)) {
            std::cerr << "[Service] startTransfer(): update image missing" << std::endl;
            reply(false);
            return;
        }

        std::thread(&FileTransferService::sendChunks, this, kUpdateImage).detach();

        std::cout << "[Service] startTransfer(): streaming " << kUpdateImage << std::endl;

        reply(true);
    }

   private:
    void sendChunks(const std::string& path) {
        std::ifstream file(path.c_str(), std::ios::binary);
        if (!file) {
            std::cerr << "[Service] Failed to open file: " << path << std::endl;
            return;
        }

        uint32_t chunkIndex = 0;
        std::vector<uint8_t> buffer(CHUNK_SIZE);

        while (true) {
            file.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE);
            std::streamsize bytesRead = file.gcount();
            if (bytesRead <= 0) break;

            bool lastChunk = (static_cast<size_t>(bytesRead) < CHUNK_SIZE);
            CommonAPI::ByteBuffer data(buffer.begin(), buffer.begin() + bytesRead);

            std::cout << "[Service] Sending chunk " << chunkIndex << " (" << bytesRead << " bytes)" << (lastChunk ? " [LAST]" : "")
                      << std::endl;

            fireFileChunkEvent(chunkIndex, data, lastChunk);
            ++chunkIndex;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "[Service] Completed sending file: " << path << std::endl;
    }
};

int main() {
    CommonAPI::Runtime::setProperty("LibraryBase", "FileTransfer");
    auto runtime = CommonAPI::Runtime::get();

    auto service = std::make_shared<FileTransferService>();

    bool ok = runtime->registerService("local", "filetransfer.example.FileTransfer", service, "service-sample");

    if (!ok) {
        std::cerr << "[Service] Failed to register service." << std::endl;
        return 1;
    }

    std::cout << "[Service] File Transfer Service running..." << std::endl;

    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
}
