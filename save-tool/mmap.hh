#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> 

#include <span>
#include <string>
#include <cstring>
#include <cstddef>
#include <stdexcept>

struct mmap_file {
    std::span<std::byte> data;
    std::string filename;
    int fd;
    mmap_file(std::string filename_):
        filename(filename_)
    {
        fd = open(filename.c_str(), O_RDWR);
        if (fd < 0) {
            throw std::runtime_error(filename + ": " + strerror(errno));
        }
        struct stat st;
        int err = fstat(fd, &st);
        if (err < 0) {
            throw std::runtime_error(filename + ": " + strerror(errno));
        }
        size_t len = st.st_size;
        void* addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            throw std::runtime_error(filename + ": " + strerror(errno));
        }

        data = {static_cast<std::byte*>(addr), len};
    }

    ~mmap_file() noexcept(false) {
        int err = munmap(data.data(), data.size());
        if (err < 0) {
            throw std::runtime_error(filename + ": " + strerror(errno));
        }
        err = close(fd);
        if (fd < 0) {
            throw std::runtime_error(filename + ": " + strerror(errno));
        }
    }
};
