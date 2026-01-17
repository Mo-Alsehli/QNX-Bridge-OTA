#include <CommonAPI/CommonAPI.hpp>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdint>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h> // for mkdir()

#include <v0/filetransfer/example/FileTransferProxy.hpp>

namespace ft = v0::filetransfer::example;

// helper to create directory if missing
void ensureClientDir() {
    struct stat st;
    if (stat("data/client", &st) != 0) {
        mkdir("data/client", 0777);
    }
}

class FileReceiver {
public:
    FileReceiver(const std::string& outputName)
        : outPath_("data/client/" + outputName) {

        ensureClientDir();

        ofs_.open(outPath_.c_str(), std::ios::binary);
        if (!ofs_) {
            std::cerr << "[Client] Failed to open output file: " << outPath_ << std::endl;
        }
    }

    void onChunk(uint32_t index, const CommonAPI::ByteBuffer& data, bool lastChunk) {
        if (!ofs_) {
            std::cerr << "[Client] Output file not open. Cannot write chunk "
                      << index << std::endl;
            return;
        }

        ofs_.write(reinterpret_cast<const char*>(data.data()),
                   static_cast<std::streamsize>(data.size()));

        std::cout << "[Client] Received Chunk " << index
                  << " (" << data.size() << " bytes)"
                  << (lastChunk ? " [LAST]" : "")
                  << std::endl;

        if (lastChunk) {
            std::cout << "[Client] All chunks received. File saved to: "
                      << outPath_ << std::endl;
            ofs_.close();
        }
    }

private:
    std::string outPath_;
    std::ofstream ofs_;
};

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: FileTransferClient <filename>" << std::endl;
        return 1;
    }

    std::string outputFilename = argv[1];

    CommonAPI::Runtime::setProperty("LibraryBase", "FileTransfer");
    auto runtime = CommonAPI::Runtime::get();

    std::shared_ptr<ft::FileTransferProxy<> > proxy;

    while (!proxy) {
        proxy = runtime->buildProxy<ft::FileTransferProxy>(
            "local",
            "filetransfer.example.FileTransfer",
            "client-sample"
        );

        if (!proxy->isAvailable()) {
            std::cout << "[Client] Service not available yet. Retrying..."
                      << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    FileReceiver receiver(outputFilename);

    proxy->getFileChunkEvent().subscribe(
        [&](const uint32_t& index,
            const CommonAPI::ByteBuffer& data,
            const bool& last) {
            receiver.onChunk(index, data, last);
        }
    );

    CommonAPI::CallStatus status;
    bool accepted = false;

    proxy->requestFile(outputFilename, status, accepted);

    if (!accepted) {
        std::cerr << "[Client] File request rejected by server." << std::endl;
        return 1;
    }

    std::cout << "[Client] Request accepted. Receiving chunks..." << std::endl;

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(1));
}
