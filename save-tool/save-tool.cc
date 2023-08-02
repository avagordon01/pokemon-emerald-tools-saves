#include "mmap.hh"

#include <iostream>
#include <span>
#include <numeric>
#include <vector>
#include <string>
#include <cassert>

void check_m_f(bool x, std::string x_str) {
    if (!x) {
        using namespace std::string_literals;
        std::string error = "failed! "s + x_str + " is false"s;
        throw std::runtime_error(error);
    }
}

#define check_m(x) check_m_f(x, #x)

template<typename A>
std::span<A> span_cast(std::span<std::byte> x) {
    assert(x.size() >= sizeof(A));
    return std::span<A>(
        reinterpret_cast<A*>(x.data()),
        x.size() / sizeof(A)
    );
}

#include "pokemon-emerald-format.hh"

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    for (auto& filename: args) {
        try {
            auto m = mmap_file(filename);
            auto d = m.data;
            if (d.size() != 32 * 4096) {
                throw std::runtime_error("wrong save file size");
            }
            auto f = span_cast<pokemon_emerald_format>(d).front();
            f.check();
            std::cout << "good pokemon save: " << filename << std::endl;
        } catch (std::runtime_error e) {
            std::cout << "error in " << filename << ": " << e.what() << std::endl;
        }
    }
}
