#include <CommonAPI/CommonAPI.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <cstdint>      // for uint32_t
#include <sys/stat.h>   // for stat()
#include <sys/types.h>

#include <v0/filetransfer/example/FileTransferStubDefault.hpp>

namespace ft = v0::filetransfer::example;

static const size_t CHUNK_SIZE = 4096; // 4KB

// Simple file-exists helper for C++14 (no filesystem)
bool fileExists(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

class FileTransferService : public ft::FileTransferStubDefault {
public:
    FileTransferService() = default;

    // MUST match generated signature:
    virtual void requestFile(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        std::string filename,
        requestFileReply_t _reply
    ) override {

        (void)_client; // unused

        std::string filePath = "data/server/" + filename;

        if (!fileExists(filePath)) {
            std::cerr << "[Service] File not found: " << filePath << std::endl;
            _reply(false);
            return;
        }

        std::cout << "[Service] Request received for file: " << filename << std::endl;
        _reply(true);

        std::thread(&FileTransferService::sendChunks, this, filePath).detach();
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

            if (bytesRead <= 0)
                break;

            bool lastChunk = (static_cast<size_t>(bytesRead) < CHUNK_SIZE);
            CommonAPI::ByteBuffer data(buffer.begin(), buffer.begin() + bytesRead);

            std::cout << "[Service] Sending Chunk " << chunkIndex
                      << " (" << bytesRead << " bytes)"
                      << (lastChunk ? " [Last]" : "") << std::endl;

            fireFileChunkEvent(chunkIndex, data, lastChunk);
            chunkIndex++;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "[Service] Completed sending file: " << path << std::endl;
    }
};

int main() {
    CommonAPI::Runtime::setProperty("LibraryBase", "FileTransfer");
    auto runtime = CommonAPI::Runtime::get();

    auto service = std::make_shared<FileTransferService>();

    bool ok = runtime->registerService(
        "local",
        "filetransfer.example.FileTransfer",
        service,
        "service-sample"
    );

    if (!ok) {
        std::cerr << "[Service] Failed to register service." << std::endl;
        return 1;
    }

    std::cout << "[Service] File Transfer Service running..." << std::endl;

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(1));
}
