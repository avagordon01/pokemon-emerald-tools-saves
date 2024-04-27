#include "mmap.hh"

#include <iostream>
#include <pthread.h>
#include <span>
#include <numeric>
#include <vector>
#include <string>
#include <cassert>

#include "pokemon-emerald-format.hh"

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.size() != 2) {
        std::cout << "usage: gift-tool save-file mystery-gift-file" << std::endl;
        return 0;
    }
    std::string filename0 = args[0];
    auto m0 = mmap_file(filename0);
    auto d0 = m0.data;
    auto& f0 = *reinterpret_cast<pokemon_emerald_format*>(d0.data());
    try {
        f0.check();
        std::cout << "good pokemon " << f0.get_game_name() << " save file: " << filename0 << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "error in " << filename0 << ": " << e.what() << std::endl;
    }
    std::string filename1 = args[1];
    auto m1 = mmap_file(filename1);
    auto d1 = m1.data;
    auto& f1 = *reinterpret_cast<mystery_gift_file_format*>(d1.data());
    try {
        f1.check();
        std::cout << "good mystery gift file: " << filename1 << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "error in " << filename1 << ": " << e.what() << std::endl;
    }
    std::cout << "writing mystery gift to save" << std::endl;
    f0.write_mystery_gift(f1);
    std::cout << "done" << std::endl;
    return 0;
}
