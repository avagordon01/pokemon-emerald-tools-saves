#include <string>
#include <array>
#include <cstddef>
#include <string_view>
#include <cuchar>
#include <climits>

std::array<char32_t, 273> pokemon_char_to_char = {
    U"                "
    U"                "
    U"                "
    U"                "
    U"                "
    U"                "
    U"                "
    U"                "
    U"                "
    U"                "
    U" 0123456789!?.- "
    U"‥“”‘'♂♀ , /ABCDE"
    U"FGHIJKLMNOPQRSTU"
    U"VWXYZabcdefghijk"
    U"lmnopqrstuvwxyz "
    U" ÄÖÜäöü         "
};

template<typename S>
std::string pokemon_string_to_string(S p_str) {
    std::u32string s32;
    for (unsigned char p_c: p_str) {
        s32.push_back(pokemon_char_to_char[p_c]);
    }

    std::string s;
    std::setlocale(LC_ALL, "en_US.utf8");
    std::mbstate_t state{};
    char out[MB_LEN_MAX]{};
    for (char32_t c: s32) {
        std::size_t rc = std::c32rtomb(out, c, &state);
        if (rc != (std::size_t) - 1) {
            for (unsigned char c8: std::string_view{out, rc}) {
                s.push_back(c8);
            }
        }
    }

    return s;
}
