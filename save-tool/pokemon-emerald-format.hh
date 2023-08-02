#include <span>
#include <array>
#include <cstddef>
#include <cstdint>

enum section_type : uint16_t {
    trainer_info,
    team_items,
    game_state,
    misc_data,
    rival_info,
    pc_buffer_a,
    pc_buffer_b,
    pc_buffer_c,
    pc_buffer_d,
    pc_buffer_e,
    pc_buffer_f,
    pc_buffer_g,
    pc_buffer_h,
    pc_buffer_i
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

struct section_game_state {
    std::array<uint8_t, 0x0405> _0;
    bool :5, mystery_event_activated: 1, :2;

    std::array<uint8_t, 5> _3;
    bool :3, mystery_gift_activated: 1, :4;

    std::array<uint8_t, 0x8e> _6;

    bool :5, eon_ticket_activated: 1, :2;
    uint8_t _7;
};
//static_assert(sizeof(section_game_state) == 3968);
static_assert(offsetof(section_game_state, _3) == 0x0405 + 1);
//static_assert(offsetof(section_game_state, mystery_gift_activated) == 0x040B);
static_assert(offsetof(section_game_state, _6) == 0x040B + 1);
static_assert(offsetof(section_game_state, _7) == 0x049A + 1);

#include "crc16_ccitt_table.hh"
/*
uint16_t crc16(std::span<uint8_t> data) {
    uint16_t v2 = 0x1121;
    for (auto x: data) {
        v2 = crc16_ccitt_table_16[(v2 ^ x) % 256] ^ (v2 >> 8);
    }
    return ~v2;
}
*/
unsigned int crc16(std::span<uint8_t> data) {
    unsigned int v2; // r2
    unsigned int i; // r3

    v2 = 0x1121; // This is the seed
    for ( i = 0; i < data.size(); i = (i + 1) & 0xFFFF )
        v2 = *(uint16_t *)((char *)crc16_ccitt_table_8 + (2 * (v2 ^ *(uint8_t *)(data.data() + i)) & 0x1FF)) ^ (v2 >> 8);
    return ~v2 & 0xFFFF;
}
/*
struct [[gnu::packed]] berry {
    uint8_t name[7];
    uint8_t firmness;
    uint16_t size_mm;
    uint8_t max_yield;
    uint8_t min_yield;
    uint32_t berry_tag_line_1_ROM_offset;
    uint32_t berry_tag_line_2_ROM_offset;
    uint8_t growth_time_per_stage_hours;
    uint8_t flavor[5];
    uint8_t smoothness;
    uint8_t _0;
    uint32_t effect_in_bag;
    uint8_t _1[14];
    uint16_t effect_as_held_item;
    uint32_t checksum;

    void check() {
        uint8_t* p = reinterpret_cast<uint8_t*>(this);
        unsigned long c = 0;
        for (size_t x = 0; x < 0x52C; x++) {
            if (x < 0xC || x >= 0x14) {
                c += p[x];
            }
        }
        check_m(checksum == c);
    }
};
static_assert(offsetof(berry, smoothness) == 0x1a);
static_assert(offsetof(berry, effect_in_bag) == 0x1c);
static_assert(offsetof(berry, effect_as_held_item) == 0x2e);
static_assert(sizeof(berry) == 52);
*/

struct section {
    std::array<std::byte, 4084> data;
    section_type section_id;
    uint16_t checksum;
    uint32_t signature;
    uint32_t save_index;

    uint16_t calculate_checksum() {
        size_t section_length = section_lengths[section_id];
        auto s = std::span(data);
        auto r = span_cast<uint32_t>(s.first(section_length));
        uint32_t count = std::accumulate(r.begin(), r.end(), 0);
        uint16_t c = (count & 0xffff) + ((count >> 16) & 0xffff);
        return c;
    }
    void check() {
        check_m(section_id < 14);
        check_m(signature == 0x08012025UL);
        check_m(checksum == calculate_checksum());
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
        bool section_ids_ordered = std::equal(sections.begin() + 1, sections.end(), sections.begin(),
            [](auto& b, auto& a){ return b.section_id == ((a.section_id + 1) % 14); });
        bool section_save_indexes_equal = std::equal(sections.begin() + 1, sections.end(), sections.begin(),
            [](auto& b, auto& a){ return b.save_index == a.save_index; });
        check_m(section_ids_ordered);
        check_m(section_save_indexes_equal);
    }

    section& get_section_by_id(section_type id) {
        section& s = sections[(14 - sections[0].section_id + id) % 14];
        assert(s.section_id == id);
        return s;
    }
};
static_assert(sizeof(game_save) == 0xE000);

struct mystery_gift_wonder_card {
    uint16_t checksum;
    uint16_t padding;
    uint16_t event_id;
    uint16_t default_icon;
    uint32_t count;
    uint8_t flag;
    uint8_t stamp_max;
    uint8_t title[40];
    uint8_t subtitle[40];
    uint8_t contents_line_1[40];
    uint8_t contents_line_2[40];
    uint8_t contents_line_3[40];
    uint8_t contents_line_4[40];
    uint8_t warning_line_1[40];
    uint8_t warning_line_2[40];
    void check() {
        auto s = std::span<uint8_t>(reinterpret_cast<uint8_t*>(this) + 4, 332);
        check_m(crc16(s) == checksum);
    }
};

struct mystery_gift_event_script {
    uint16_t checksum;
    uint16_t padding;
    std::array<uint8_t, 1000> event_script;
    void check() {
        check_m(crc16(this->event_script) == checksum);
    }
};

struct mystery_gift_file_format {
    mystery_gift_wonder_card wonder_card;
    std::array<uint8_t, 10> padding0;
    uint16_t icon;
    std::array<uint8_t, 68> padding1;
    mystery_gift_event_script event_script;

    void check() {
        wonder_card.check();
        event_script.check();
    }
};

static_assert(sizeof(mystery_gift_file_format::wonder_card) == 336);
static_assert(sizeof(mystery_gift_file_format::icon) == 2);
static_assert(sizeof(mystery_gift_file_format::event_script) == 1004);
static_assert(offsetof(mystery_gift_file_format, wonder_card) == 0);
static_assert(offsetof(mystery_gift_file_format, icon) == 346);
static_assert(offsetof(mystery_gift_file_format, event_script) == 416);

struct mystery_gift_save_format_emerald {
    std::array<uint8_t, 1388> padding0;
    mystery_gift_wonder_card wonder_card;
    std::array<uint8_t, 10> padding1;
    uint16_t icon;
    std::array<uint8_t, 480> padding2;
    mystery_gift_event_script event_script;
};

static_assert(offsetof(mystery_gift_save_format_emerald, wonder_card) == 1388);
static_assert(offsetof(mystery_gift_save_format_emerald, icon) == 1734);
static_assert(offsetof(mystery_gift_save_format_emerald, event_script) == 2216);

struct mystery_gift_save_format_frlg {
    std::array<uint8_t, 1120> padding0;
    mystery_gift_wonder_card wonder_card;
    std::array<uint8_t, 10> padding1;
    uint16_t icon;
    std::array<uint8_t, 480> padding2;
    mystery_gift_event_script event_script;
};

static_assert(offsetof(mystery_gift_save_format_frlg, wonder_card) == 1120);
static_assert(offsetof(mystery_gift_save_format_frlg, icon) == 1466);
static_assert(offsetof(mystery_gift_save_format_frlg, event_script) == 1948);



struct pokemon_emerald_format {
    game_save a;
    game_save b;
    std::array<uint8_t, 8192> hall_of_fame;
    std::array<uint8_t, 4096> mystery_gift;
    std::array<uint8_t, 4096> recorded_battle;

    void check() {
        if (a.sections.back().save_index != 0xffffffff) {
            a.check();
        }
        b.check();
    }

    game_save& get_latest_game_save() {
        if (a.sections.back().save_index > b.sections.back().save_index) {
            return a;
        } else {
            return b;
        }
    }

    std::string get_game_name() {
        std::byte game_id = get_latest_game_save().get_section_by_id(section_type{0}).data[0xac];
        if (game_id == std::byte{0}) {
            return "ruby or sapphire";
        } else if (game_id == std::byte{1}) {
            return "fire red or leaf green";
        } else {
            return "emerald";
        }
    }

    void write_mystery_gift(mystery_gift_file_format& mg) {
        section& s = get_latest_game_save().get_section_by_id(section_type{4});
        if (get_game_name() == "fire red or leaf green") {
            auto d = reinterpret_cast<mystery_gift_save_format_frlg*>(s.data.data());
            d->wonder_card = mg.wonder_card;
            d->icon = mg.icon;
            d->event_script = mg.event_script;
        } else if (get_game_name() == "emerald") {
            auto d = reinterpret_cast<mystery_gift_save_format_emerald*>(s.data.data());
            d->wonder_card = mg.wonder_card;
            d->icon = mg.icon;
            d->event_script = mg.event_script;
        }
        s.checksum = s.calculate_checksum();
    }
};

static_assert(offsetof(pokemon_emerald_format, a) == 0);
static_assert(offsetof(pokemon_emerald_format, b) == 0xE000);
static_assert(sizeof(pokemon_emerald_format) == 128 * 1024);
