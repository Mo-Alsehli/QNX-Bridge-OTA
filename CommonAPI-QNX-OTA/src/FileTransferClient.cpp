#include <sys/stat.h>
#include <sys/types.h>
<<<<<<< HEAD
#include <unistd.h>  
=======
#include <unistd.h>  // mkdir()
>>>>>>> 1a45b159ec1311430e83101bd8d3df1eae984571

#include <CommonAPI/CommonAPI.hpp>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <v0/filetransfer/example/FileTransferProxy.hpp>

namespace ft = v0::filetransfer::example;
<<<<<<< HEAD
static const size_t CHUNK_SIZE = 64 * 1024;  // 64KB
static size_t UPDATE_SIZE;
ft::FileTransfer::UpdateInfo info;
=======
>>>>>>> 1a45b159ec1311430e83101bd8d3df1eae984571

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

<<<<<<< HEAD
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
=======
>>>>>>> 1a45b159ec1311430e83101bd8d3df1eae984571

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

<<<<<<< HEAD
        double progress = (static_cast<double>(index * CHUNK_SIZE) / static_cast<double>(info.getSize())) * 100.0;

        std::cout << "\r[Client] Downloading " << static_cast<int>(progress) << "%" << std::flush;

        ofs_.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

        // std::cout << "[Client] Received Chunk " << index << " (" << data.size() << " bytes)" << (lastChunk ? " [LAST]" : "") <<
        // std::endl;
=======
        ofs_.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

        //std::cout << "[Client] Received Chunk " << index << " (" << data.size() << " bytes)" << (lastChunk ? " [LAST]" : "") << std::endl;
>>>>>>> 1a45b159ec1311430e83101bd8d3df1eae984571

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

<<<<<<< HEAD
    std::shared_ptr<ft::FileTransferProxy<>> proxy;
=======
    std::shared_ptr<ft::FileTransferProxy<> > proxy;
>>>>>>> 1a45b159ec1311430e83101bd8d3df1eae984571

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

<<<<<<< HEAD
    uint32_t currentVersion = 0;
    readUint32FromFile("data/client/update.version", currentVersion);
    CommonAPI::CallStatus status;

    proxy->requestUpdate(currentVersion, status, info);
    UPDATE_SIZE = info.getSize();
=======
    uint32_t currentVersion = 1;
    CommonAPI::CallStatus status;
    ft::FileTransfer::UpdateInfo info;

    proxy->requestUpdate(currentVersion, status, info);
>>>>>>> 1a45b159ec1311430e83101bd8d3df1eae984571

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
<<<<<<< HEAD
    std::cout << "[Client] Info - New Version: " << info.getNewVersion() << ", Size: " << info.getSize() << ", CRC: 0x" << std::hex
              << info.getCrc() << std::dec << ", Result Code: " << info.getResultCode() << std::endl;
=======
    std::cout << "[Client] Info - New Version: " << info.getNewVersion()
              << ", Size: " << info.getSize()
              << ", CRC: 0x" << std::hex << info.getCrc() << std::dec
              << ", Result Code: " << info.getResultCode()
              << std::endl;
>>>>>>> 1a45b159ec1311430e83101bd8d3df1eae984571
    proxy->startTransfer("qnx_uefi.iso", status, accepted);

    if (!accepted) {
        std::cerr << "[Client] startTransfer rejected!" << std::endl;
        return 1;
    }

    std::cout << "[Client] Receiving chunks..." << std::endl;

    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
}
