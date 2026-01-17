#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/fs.h> // BLKGETSIZE64
#include <cstdint>

#define QEMU_ENV 1

namespace {

enum class Slot { A, B };

struct Plan {
    Slot active;
    Slot target;
    std::string targetDev;   // /dev/mmcblk0p2 or p3
    std::string targetLabel; // rootfsA or rootfsB
};


#if QEMU_ENV == 0
static const char* kDevA = "/dev/mmcblk0p2";
static const char* kDevB = "/dev/mmcblk0p3";
static const char* kLabelA = "rootfsA";
static const char* kLabelB = "rootfsB";
#else
static const char* kDevA = "/dev/vda2";
static const char* kDevB = "/dev/vda3";
static const char* kLabelA = "rootfsA";
static const char* kLabelB = "rootfsB";
#endif


std::string readTextFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Failed to open " + path + ": " + std::strerror(errno));
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool isRoot() {
    return geteuid() == 0;
}

std::optional<Slot> detectActiveSlotFromCmdline(const std::string& cmdline) {
    // Expected patterns:
    // root=PARTLABEL=rootfsA
    // root=PARTLABEL=rootfsB
    if (cmdline.find("root=PARTLABEL=rootfsA") != std::string::npos) return Slot::A;
    if (cmdline.find("root=PARTLABEL=rootfsB") != std::string::npos) return Slot::B;

    // Some systems might use root=LABEL= or root=/dev/...
        // QEMU / virtio fallback
    if (cmdline.find("root=/dev/vda2") != std::string::npos) return Slot::A;
    if (cmdline.find("root=/dev/vda3") != std::string::npos) return Slot::B;
    return std::nullopt;
}

Plan buildPlan(Slot active) {
    Plan p{};
    p.active = active;
    p.target = (active == Slot::A) ? Slot::B : Slot::A;

    if (p.target == Slot::A) {
        p.targetDev = kDevA;
        p.targetLabel = kLabelA;
    } else {
        p.targetDev = kDevB;
        p.targetLabel = kLabelB;
    }
    return p;
}

uint64_t fileSizeBytes(const std::string& path) {
    struct stat st{};
    if (stat(path.c_str(), &st) != 0) {
        throw std::runtime_error("stat(" + path + ") failed: " + std::strerror(errno));
    }
    if (!S_ISREG(st.st_mode)) {
        throw std::runtime_error("Update image is not a regular file: " + path);
    }
    return static_cast<uint64_t>(st.st_size);
}

uint64_t blockDeviceSizeBytes(const std::string& devPath) {
    int fd = open(devPath.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        throw std::runtime_error("Failed to open block device " + devPath + ": " + std::strerror(errno));
    }
    uint64_t bytes = 0;
    if (ioctl(fd, BLKGETSIZE64, &bytes) != 0) {
        close(fd);
        throw std::runtime_error("ioctl(BLKGETSIZE64) failed for " + devPath + ": " + std::strerror(errno));
    }
    close(fd);
    return bytes;
}

void fsyncFile(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return;
    fsync(fd);
    close(fd);
}

void atomicWriteFile(const std::string& path, const std::string& content) {
    // Write to temp file in same directory then rename()
    std::string tmp = path + ".tmp";
    int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) {
        throw std::runtime_error("Failed to open temp file " + tmp + ": " + std::strerror(errno));
    }

    ssize_t total = 0;
    const char* buf = content.data();
    ssize_t len = static_cast<ssize_t>(content.size());
    while (total < len) {
        ssize_t n = write(fd, buf + total, len - total);
        if (n < 0) {
            close(fd);
            throw std::runtime_error("Write failed to " + tmp + ": " + std::strerror(errno));
        }
        total += n;
    }

    if (fsync(fd) != 0) {
        close(fd);
        throw std::runtime_error("fsync failed for " + tmp + ": " + std::strerror(errno));
    }
    close(fd);

    if (rename(tmp.c_str(), path.c_str()) != 0) {
        throw std::runtime_error("rename(" + tmp + " -> " + path + ") failed: " + std::strerror(errno));
    }

    // Best effort to fsync directory entry
    auto slash = path.find_last_of('/');
    if (slash != std::string::npos) {
        std::string dir = path.substr(0, slash);
        int dfd = open(dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (dfd >= 0) {
            fsync(dfd);
            close(dfd);
        }
    }
}

std::string updateExtlinuxRootPartlabel(const std::string& extlinux, const std::string& newLabel) {
    // Replace only "root=PARTLABEL=rootfsA" or "root=PARTLABEL=rootfsB"
    // Keep everything else the same.
    std::string out = extlinux;

    const std::string a = "root=PARTLABEL=rootfsA";
    const std::string b = "root=PARTLABEL=rootfsB";
    size_t posA = out.find(a);
    size_t posB = out.find(b);

    if (posA == std::string::npos && posB == std::string::npos) {
        throw std::runtime_error("extlinux.conf does not contain root=PARTLABEL=rootfsA/B");
    }

    const std::string repl = "root=PARTLABEL=" + newLabel;

    if (posA != std::string::npos) out.replace(posA, a.size(), repl);
    if (posB != std::string::npos) out.replace(posB, b.size(), repl);
    return out;
}

void streamWriteImageToBlock(const std::string& imagePath, const std::string& devPath) {
    int in = open(imagePath.c_str(), O_RDONLY | O_CLOEXEC);
    if (in < 0) {
        throw std::runtime_error("Failed to open image " + imagePath + ": " + std::strerror(errno));
    }

    int out = open(devPath.c_str(), O_WRONLY | O_CLOEXEC);
    if (out < 0) {
        close(in);
        throw std::runtime_error("Failed to open target device " + devPath + ": " + std::strerror(errno));
    }

    constexpr size_t kBuf = 4 * 1024 * 1024;
    std::string buffer;
    buffer.resize(kBuf);

    uint64_t written = 0;
    while (true) {
        ssize_t r = read(in, buffer.data(), buffer.size());
        if (r == 0) break;
        if (r < 0) {
            close(in); close(out);
            throw std::runtime_error("Read failed: " + std::string(std::strerror(errno)));
        }

        ssize_t off = 0;
        while (off < r) {
            ssize_t w = write(out, buffer.data() + off, r - off);
            if (w < 0) {
                close(in); close(out);
                throw std::runtime_error("Write to block device failed: " + std::string(std::strerror(errno)));
            }
            off += w;
            written += static_cast<uint64_t>(w);
        }

        // Minimal progress
        if ((written % (256ULL * 1024 * 1024)) < kBuf) {
            std::cerr << "Written " << (written / (1024 * 1024)) << " MiB...\n";
        }
    }

    if (fsync(out) != 0) {
        close(in); close(out);
        throw std::runtime_error("fsync(target) failed: " + std::string(std::strerror(errno)));
    }

    close(in);
    close(out);

    // Ensure kernel flushes everything
    sync();
}

struct Args {
    std::string image;
    std::string bootconf = "/boot/extlinux/extlinux.conf";
    bool dryRun = false;
};

Args parseArgs(int argc, char** argv) {
    Args a{};
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto needValue = [&](const std::string& opt) -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("Missing value for " + opt);
            return argv[++i];
        };

        if (k == "--image") a.image = needValue(k);
        else if (k == "--bootconf") a.bootconf = needValue(k);
        else if (k == "--dry-run") a.dryRun = true;
        else if (k == "-h" || k == "--help") {
            std::cout <<
                "Usage: ota-apply --image <rootfs.ext4> [--bootconf <path>] [--dry-run]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + k);
        }
    }

    if (a.image.empty()) throw std::runtime_error("Missing required: --image <path>");
    return a;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (!isRoot()) {
            std::cerr << "ERROR: ota-apply must run as root.\n";
            return 10;
        }

        Args args = parseArgs(argc, argv);

        // 1) Determine active slot
        std::string cmdline = readTextFile("/proc/cmdline");
        auto activeOpt = detectActiveSlotFromCmdline(cmdline);
        if (!activeOpt) {
            std::cerr << "ERROR: Cannot determine active slot from /proc/cmdline\n";
            return 20;
        }
        Plan plan = buildPlan(*activeOpt);

        std::cerr << "Active slot: " << ((*activeOpt == Slot::A) ? "A" : "B")
                  << "  Target slot: " << ((plan.target == Slot::A) ? "A" : "B") << "\n";
        std::cerr << "Target device: " << plan.targetDev
                  << "  Target label: " << plan.targetLabel << "\n";

        // 2) Validate image
        uint64_t imgBytes = fileSizeBytes(args.image);
        uint64_t devBytes = blockDeviceSizeBytes(plan.targetDev);

        std::cerr << "Image size: " << (imgBytes / (1024 * 1024)) << " MiB\n";
        std::cerr << "Target size: " << (devBytes / (1024 * 1024)) << " MiB\n";

        if (imgBytes > devBytes) {
            std::cerr << "ERROR: Image is larger than target partition.\n";
            return 30;
        }

        // 3) Update extlinux.conf content (prepare first)
        std::string extlinux = readTextFile(args.bootconf);
        std::string updated = updateExtlinuxRootPartlabel(extlinux, plan.targetLabel);

        if (args.dryRun) {
            std::cerr << "DRY RUN: would write image to " << plan.targetDev
                      << " and update " << args.bootconf << "\n";
            return 0;
        }

        // 4) Write image to inactive partition
        std::cerr << "Writing image to " << plan.targetDev << "...\n";
        streamWriteImageToBlock(args.image, plan.targetDev);
        std::cerr << "Write complete.\n";

        // 5) Update extlinux.conf atomically (after successful write)
        std::cerr << "Updating boot config " << args.bootconf << "...\n";
        atomicWriteFile(args.bootconf, updated);
        fsyncFile(args.bootconf);
        sync();
        std::cerr << "Boot config updated.\n";

        std::cerr << "SUCCESS: Update applied to inactive slot.\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 50;
    }
}