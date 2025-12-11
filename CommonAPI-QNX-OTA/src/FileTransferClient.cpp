#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>  // mkdir()

#include <CommonAPI/CommonAPI.hpp>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <v0/filetransfer/example/FileTransferProxy.hpp>

namespace ft = v0::filetransfer::example;

// helper to create directory if missing
void ensureClientDir() {
    struct stat st;

    if (stat("data/client", &st) != 0) {
        mkdir("data/client", 0777);
    }

    struct stat vf;
    if (stat("data/client/update.version", &vf) != 0) {
        std::ofstream versionFile("data/client/update.version");
        if (versionFile.is_open()) {
            versionFile << "0";
            versionFile.close();
        } else {
            std::cerr << "[Client] Failed to create version file!" << std::endl;
        }
    }
}


class FileReceiver {
   public:
    FileReceiver(const std::string& outputName) : outPath_("data/client/" + outputName) {
        ensureClientDir();

        ofs_.open(outPath_.c_str(), std::ios::binary);
        if (!ofs_) {
            std::cerr << "[Client] Failed to open output file: " << outPath_ << std::endl;
        }
    }

    void onChunk(uint32_t index, const CommonAPI::ByteBuffer& data, bool lastChunk) {
        if (!ofs_) {
            std::cerr << "[Client] Output file not open. Cannot write chunk " << index << std::endl;
            return;
        }

        ofs_.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

        //std::cout << "[Client] Received Chunk " << index << " (" << data.size() << " bytes)" << (lastChunk ? " [LAST]" : "") << std::endl;

        if (lastChunk) {
            std::cout << "[Client] All chunks received. File saved to: " << outPath_ << std::endl;
            ofs_.close();
        }
    }

   private:
    std::string outPath_;
    std::ofstream ofs_;
};

int main() {
    std::string outputFilename = "qnx_uefi.iso";

    CommonAPI::Runtime::setProperty("LibraryBase", "FileTransfer");
    auto runtime = CommonAPI::Runtime::get();

    std::shared_ptr<ft::FileTransferProxy<> > proxy;

    while (!proxy) {
        proxy = runtime->buildProxy<ft::FileTransferProxy>("local", "filetransfer.example.FileTransfer", "client-sample");

        if (!proxy->isAvailable()) {
            std::cout << "[Client] Waiting for service..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    FileReceiver receiver(outputFilename);

    proxy->getFileChunkEvent().subscribe(
        [&](uint32_t index, const CommonAPI::ByteBuffer& data, bool last) { receiver.onChunk(index, data, last); });

    uint32_t currentVersion = 1;
    CommonAPI::CallStatus status;
    ft::FileTransfer::UpdateInfo info;

    proxy->requestUpdate(currentVersion, status, info);

    if (status != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "[Client] requestUpdate failed!" << std::endl;
        return 1;
    }

    if (!info.getExists()) {
        std::cout << "[Client] No update on server." << std::endl;
        return 0;
    }

    if (!info.getIsNew()) {
        std::cout << "[Client] Already up to date." << std::endl;
        return 0;
    }

    std::cout << "[Client] New update available. Starting transfer..." << std::endl;

    bool accepted = false;
    std::cout << "[Client] Info - New Version: " << info.getNewVersion()
              << ", Size: " << info.getSize()
              << ", CRC: 0x" << std::hex << info.getCrc() << std::dec
              << ", Result Code: " << info.getResultCode()
              << std::endl;
    proxy->startTransfer("qnx_uefi.iso", status, accepted);

    if (!accepted) {
        std::cerr << "[Client] startTransfer rejected!" << std::endl;
        return 1;
    }

    std::cout << "[Client] Receiving chunks..." << std::endl;

    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
}
