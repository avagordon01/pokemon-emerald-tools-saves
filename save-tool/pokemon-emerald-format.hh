#include <span>
#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>

#include "util.hh"
#include "pokemon-names.hh"
#include "species-id-conversion.hh"

enum game_version : uint32_t {
    ruby_sapphire,
    leafgreen_firered,
    emerald,
};

std::ostream& operator<<(std::ostream& os, enum game_version gv) {
    switch (gv) {
        case ruby_sapphire:
            os << "ruby/sapphire";
            break;
        case leafgreen_firered:
            os << "leaf green/fire red";
            break;
        case emerald:
            os << "emerald";
            break;
        default:
            os << "unknown game_version";
    }
    return os;
}

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

struct section_trainer_info {
    std::array<uint8_t, 0xac> _;
    uint32_t game_code;

    enum game_version game_version() const {
        switch (game_code) {
            case 0x00000000:
                return game_version::ruby_sapphire;
            case 0x00000001:
                return game_version::leafgreen_firered;
            default:
                return game_version::emerald;
        }
    }
};

std::array<size_t, 3> team_size_offsets = {0x234, 0x34, 0x234};
std::array<size_t, 3> team_list_offsets = {0x238, 0x38, 0x238};

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
uint16_t crc16(std::span<uint8_t> data) {
    uint16_t v2 = 0x1121;
    for (auto x: data) {
        v2 = crc16_ccitt_table_16[(v2 ^ x) % 256] ^ (v2 >> 8);
    }
    return ~v2;
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


struct pokemon_data_growth {
    uint16_t species;
    uint16_t item_held;
    uint32_t experience;
    uint8_t pp_bonuses;
    uint8_t friendship;
    uint16_t _;
};
struct pokemon_data_attacks {
    std::array<uint16_t, 4> moves;
    std::array<uint8_t, 4> pp;
};
struct pokemon_data_evs_condition {
    uint8_t hp_ev;
    uint8_t attack_ev;
    uint8_t defense_ev;
    uint8_t speed_ev;
    uint8_t sp_attack_ev;
    uint8_t sp_defense_ev;
    uint8_t coolness;
    uint8_t beauty;
    uint8_t cuteness;
    uint8_t smartness;
    uint8_t toughness;
    uint8_t feel;
};
struct pokemon_data_misc {
    uint8_t pokerus;
    uint8_t met_location;
    uint16_t origins_info;
    uint32_t iv_egg_ability;
    uint32_t ribbons_obedience;
};

static_assert(sizeof(pokemon_data_growth) == 12);
static_assert(sizeof(pokemon_data_attacks) == 12);
static_assert(sizeof(pokemon_data_evs_condition) == 12);
static_assert(sizeof(pokemon_data_misc) == 12);
        
std::array<std::array<uint8_t, 4>, 24> pokemon_data_orders = {{
    {0, 1, 2, 3},
    {0, 1, 3, 2},
    {0, 2, 1, 3},
    {0, 3, 1, 2},
    {0, 2, 3, 1},
    {0, 3, 2, 1},
    {1, 0, 2, 3},
    {1, 0, 3, 2},
    {2, 0, 1, 3},
    {3, 0, 1, 2},
    {2, 0, 3, 1},
    {3, 0, 2, 1},
    {1, 2, 0, 3},
    {1, 3, 0, 2},
    {2, 1, 0, 3},
    {3, 1, 0, 2},
    {2, 3, 0, 1},
    {3, 2, 0, 1},
    {1, 2, 3, 0},
    {1, 3, 2, 0},
    {2, 1, 3, 0},
    {3, 1, 2, 0},
    {2, 3, 1, 0},
    {3, 2, 1, 0},
}};

/*
std::array<std::array<uint8_t, 4>, 24> pokemon_data_orders = {{
    {0, 1, 2, 3},
    {0, 1, 3, 2},
    {0, 2, 1, 3},
    {0, 2, 3, 1},
    {0, 3, 1, 2},
    {0, 3, 2, 1},
    {1, 0, 2, 3},
    {1, 0, 3, 2},
    {1, 2, 0, 3},
    {1, 2, 3, 0},
    {1, 3, 0, 2},
    {1, 3, 2, 0},
    {2, 0, 1, 3},
    {2, 0, 3, 1},
    {2, 1, 0, 3},
    {2, 1, 3, 0},
    {2, 3, 0, 1},
    {2, 3, 1, 0},
    {3, 0, 1, 2},
    {3, 0, 2, 1},
    {3, 1, 0, 2},
    {3, 1, 2, 0},
    {3, 2, 0, 1},
    {3, 2, 1, 0},
}};
*/

struct pokemon_box {
    uint32_t personality;
    uint32_t original_trainer_id;
    std::array<char, 10> nickname;
    uint8_t language;
    uint8_t misc_flags;
    std::array<char, 7> original_trainer_name;
    uint8_t markings;
    uint16_t checksum;
    uint16_t _;
    union {
        std::array<pokemon_data_growth, 4> data;
        struct {
            pokemon_data_growth growth;
            pokemon_data_attacks attacks;
            pokemon_data_evs_condition evs_condition;
            pokemon_data_misc misc;
        };
    };

    bool empty() {
        return growth.species == 0 or personality == 0 or checksum == 0xffff;
    }

    void decode() {
        uint8_t index = personality % 24;
        std::array<uint8_t, 4> order = pokemon_data_orders[index];
        for (uint8_t i = 0; i < 4; i++) {
            if (order[i] == i) {
                continue;
            } else {
                std::swap(data[i], data[order[i]]);
                std::swap(order[i], order[order[i]]);
            }
        }
        //check_m(std::is_sorted(order.begin(), order.end()));
        uint32_t decryption_key = original_trainer_id ^ personality;
        xor_bytes(span_bytes<pokemon_data_growth>(std::span(data)), decryption_key);
    }
    void check() {
        std::span<uint16_t, sizeof(data) / sizeof(uint16_t)> s{reinterpret_cast<uint16_t*>(&data), sizeof(data) / sizeof(uint16_t)};

        uint16_t total = 0;
        for (auto x: s) {
            total += x;
        }
        assert(total == checksum);
    }

    const std::string_view species_name() const {
        uint16_t national_id = internal_to_national(growth.species);
        try {
        return pokemon_names.at(national_id - 1);
        } catch (const std::exception& e) {
            return "error";
        }
    }
};
struct pokemon_party: pokemon_box {
    uint32_t status_condition;
    uint8_t level;
    uint8_t mail_id;
    uint16_t current_hp;
    uint16_t total_hp;
    uint16_t attack;
    uint16_t defense;
    uint16_t speed;
    uint16_t sp_attack;
    uint16_t sp_defense;
};

std::ostream& operator<<(std::ostream& os, const pokemon_party& p) {
    const auto internal = p.growth.species;
    const auto national = internal_to_national(internal);
    os << "i = " << internal << ", n = " << national << ", " << p.species_name() << std::endl;
    os << "personality = " << p.personality << ", experience = " << p.growth.experience;
    return os;
}

std::ostream& operator<<(std::ostream& os, const pokemon_box& p) {
    const auto internal = p.growth.species;
    const auto national = internal_to_national(internal);
    os << "i = " << internal << ", n = " << national << ", " << p.species_name() << std::endl;
    os << "personality = " << p.personality << ", experience = " << p.growth.experience;
    return os;
}

static_assert(sizeof(pokemon_party) == 100);
static_assert(sizeof(pokemon_box) == 80);

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
