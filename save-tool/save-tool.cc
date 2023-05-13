#include "mmap.hh"

#include <iostream>
#include <span>
#include <numeric>
#include <vector>
#include <string>
#include <cassert>

template<typename X, typename Y>
void check_eq_f(X x, Y y, std::string x_str, std::string y_str) {
    if (x != y) {
        using namespace std::string_literals;
        std::string error = "error! "s + x_str + " ("s + std::to_string(x) + ") != "s + y_str + " ("s + std::to_string(y) + ")"s;
        throw std::runtime_error(error);
    }
}

#define check_eq(x, y) check_eq_f(x, y, #x, #y)

template<typename A>
std::span<A> span_cast(std::span<std::byte> x) {
    assert(x.size() >= sizeof(A));
    return std::span<A>(
        reinterpret_cast<A*>(x.data()),
        x.size() / sizeof(A)
    );
}

enum section_type : uint16_t {
};

std::array<size_t, 14> section_lengths = {
    3884,
    3968,
    3968,
    3968,
    3848,
    3968,
    3968,
    3968,
    3968,
    3968,
    3968,
    3968,
    3968,
    2000
};

struct section {
    std::array<std::byte, 4084> data;
    section_type section_id;
    uint16_t checksum;
    uint32_t signature;
    uint32_t save_index;

    void check() {
        check_eq(signature, 0x08012025UL);
        {
            size_t section_length = section_lengths[section_id];
            auto s = std::span(data);
            auto r = span_cast<uint32_t>(s.first(section_length));
            uint32_t count = std::accumulate(r.begin(), r.end(), 0);
            uint16_t c = (count & 0xffff) + ((count >> 16) & 0xffff);
            check_eq(checksum, c);
        }
    }
};
static_assert(offsetof(section, signature) == 0x0FF8);
static_assert(sizeof(section) == 4 * 1024);

struct game_save {
    std::array<section, 14> sections;

    void check() {
        for (auto& section: sections) {
            section.check();
        }
        check_eq(std::equal(sections.begin() + 1, sections.end(), sections.begin(),
            [](auto& b, auto& a){ return b.section_id == ((a.section_id + 1) % 14); }), true);
        check_eq(std::equal(sections.begin() + 1, sections.end(), sections.begin(),
            [](auto& b, auto& a){ return b.save_index == a.save_index; }), true);
    }
};
static_assert(sizeof(game_save) == 0xE000);

struct file {
    game_save a;
    game_save b;
    //hall_of_fame hall_of_fame;
    //mystery_gift mystery_gift;
    //recorded_battle;

    void check() {
        if (a.sections.back().save_index != 0xffffffff) {
            a.check();
        }
        b.check();
    }
};
static_assert(offsetof(file, a) == 0);
static_assert(offsetof(file, b) == 0xE000);

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    for (auto& filename: args) {
        try {
            auto m = mmap_file(filename);
            auto d = m.data;
            if (!(d.size() == 28 * 4096 || d.size() == 30 * 4096 || d.size() == 31 * 4096 || d.size() == 32 * 4096)) {
                throw std::runtime_error("wrong save file size");
            }
            auto f = span_cast<file>(d).front();
            f.check();
            std::cout << "good pokemon save: " << filename << std::endl;
        } catch (std::runtime_error e) {
            std::cout << "error in " << filename << ": " << e.what() << std::endl;
        }
    }
}
