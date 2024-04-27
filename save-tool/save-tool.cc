#include "mmap.hh"

#include <iostream>
#include <span>
#include <numeric>
#include <vector>
#include <string>
#include <cassert>

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
        } catch (const std::runtime_error& e) {
            std::cout << "error in " << filename << ": " << e.what() << std::endl;
        }
    }
}
