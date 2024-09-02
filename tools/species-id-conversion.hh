#include <cstdint>
#include <array>

constexpr uint16_t first_unaligned_national = 252;
constexpr uint16_t first_unaligned_internal = 277;

constexpr std::array<int8_t, 135> national_to_internal_table = {{
               25,  25,  25,  25,  25,  25,  25,  25,
     25,  25,  25,  25,  25,  25,  25,  25,  25,  25,
     25,  25,  25,  25,  25,  25,  28,  28,  31,  31,
    112, 112, 112,  28,  28,  21,  21,  77,  77,  77,
     11,  11,  11,  77,  77,  77,  39,  39,  52,  21,
     15,  15,  20,  52,  78,  78,  78,  49,  49,  28,
     28,  42,  42,  73,  73,  48,  51,  51,  12,  12,
     -7,  -7,  17,  17,  -3,  26,  26, -19,   4,   4,
      4,  13,  13,  25,  25,  45,  43,  11,  11, -16,
    -16, -15, -15, -25, -25,  43,  43,  43,  43, -21,
    -21,  34, -35,  24,  24,   6,   6,  12,  53,  17,
      0, -15, -15, -22, -22, -22,   7,   7,   7,  12,
    -45,  24,  24,  24,  24,  24,  24,  24,  24,  24,
     27,  27,  22,  22,  22,  24,  24,
}};

constexpr std::array<int8_t, 135> internal_to_national_table = {{
                                       -25, -25, -25,
    -25, -25, -25, -25, -25, -25, -25, -25, -25, -25,
    -25, -25, -25, -25, -25, -25, -25, -25, -25, -25,
    -25, -11, -11, -11, -28, -28, -21, -21,  19, -31,
    -31, -28, -28,   7,   7, -15, -15,  35,  25,  25,
    -21,   3, -20,  16,  16,  45,  15,  15,  21,  21,
    -12, -12,  -4,  -4,  -4, -39, -39, -28, -28, -17,
    -17,  22,  22,  22, -13, -13,  15,  15, -11, -11,
    -52, -26, -26, -42, -42, -52, -49, -49, -25, -25,
      0,  -6,  -6, -48, -77, -77, -77, -51, -51, -12,
    -77, -77, -77,  -7,  -7,  -7, -17, -24, -24, -43,
    -45, -12, -78, -78, -78, -34, -73, -73, -43, -43,
    -43, -43,-112,-112,-112, -24, -24, -24, -24, -24,
    -24, -24, -24, -24, -22, -22, -22, -27, -27, -24,
    -24, -53,
}};

constexpr uint16_t national_to_internal(uint16_t national) {
    uint16_t shift = national - first_unaligned_national;
    const auto& table = national_to_internal_table;
    if (shift >= table.size()) {
        return national;
    }
    return national + table[shift];
}

constexpr uint16_t internal_to_national(uint16_t internal) {
    if (internal < first_unaligned_national)
        return internal;
    uint16_t shift = internal - first_unaligned_internal;
    const auto& table = internal_to_national_table;
    if (shift >= table.size()) {
        return 0;
    }
    return internal + table[shift];
}

static_assert(internal_to_national(282) == 257);
static_assert(national_to_internal(257) == 282);
static_assert(national_to_internal(internal_to_national(0)) == 0);
static_assert(national_to_internal(internal_to_national(50)) == 50);
static_assert(national_to_internal(internal_to_national(100)) == 100);
static_assert(national_to_internal(internal_to_national(150)) == 150);
static_assert(national_to_internal(internal_to_national(200)) == 200);
static_assert(national_to_internal(internal_to_national(250)) == 250);
static_assert(national_to_internal(internal_to_national(300)) == 300);