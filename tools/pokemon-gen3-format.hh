#include <span>
#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <utility>

#include "util.hh"
#include "pokemon-names.hh"
#include "species-id-conversion.hh"
#include "pokemon-strings.hh"
#include "pokemon-levelling.hh"

enum game_version : uint32_t {
    ruby_sapphire,
    leafgreen_firered,
    emerald,
};

const std::array<std::string, 4> game_version_strings = {
    "ruby/sapphire",
    "leaf green/fire red",
    "emerald",
    "unknown game_version",
};

std::ostream& operator<<(std::ostream& os, enum game_version gv) {
    os << game_version_strings[std::min(static_cast<size_t>(gv), game_version_strings.size() - 1)];
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

constexpr auto num_sections = 14;

std::array<size_t, num_sections> section_lengths = {
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
uint16_t crc16(std::span<std::byte> data) {
    uint16_t v2 = 0x1121;
    for (auto x: data) {
        v2 = crc16_ccitt_table_16[(v2 ^ static_cast<uint8_t>(x)) % 256] ^ (v2 >> 8);
    }
    return ~v2;
}

uint16_t block_checksum(std::span<uint32_t> data) {
    uint32_t sum = std::accumulate(data.begin(), data.end(), 0);
    return sum + (sum >> 16);
}

uint16_t block_checksum(std::span<std::byte> data) {
    return block_checksum(span_cast<uint32_t>(data));
}

struct section {
    std::array<std::byte, 4084> data;
    section_type section_id;
    uint16_t checksum;
    uint32_t signature;
    uint32_t save_index;

    std::span<std::byte> data_span() {
        return std::span(data.begin(), data.begin() + section_lengths[section_id]);
    }

    uint16_t calculate_checksum() {
        size_t section_length = section_lengths[section_id];
        auto s = std::span(data).first(section_length);
        return block_checksum(s);
    }
    void check() {
        check_m(section_id < num_sections);
        check_m(signature == 0x08012025UL);
        check_m(checksum == calculate_checksum());
    }
};
static_assert(offsetof(section, signature) == 0x0FF8);
static_assert(sizeof(section) == 4 * 1024);

struct game_save {
    std::array<section, num_sections> sections;

    void check() {
        for (auto& section: sections) {
            section.check();
        }
        bool section_ids_ordered = std::equal(sections.begin() + 1, sections.end(), sections.begin(),
            [](auto& b, auto& a){ return b.section_id == ((a.section_id + 1) % num_sections); });
        bool section_save_indexes_equal = std::equal(sections.begin() + 1, sections.end(), sections.begin(),
            [](auto& b, auto& a){ return b.save_index == a.save_index; });
        check_m(section_ids_ordered);
        check_m(section_save_indexes_equal);
    }

    section& get_section_by_id(section_type id) {
        section& s = sections[(num_sections - sections[0].section_id + id) % num_sections];
        assert(s.section_id == id);
        return s;
    }

    std::vector<std::byte> get_sections_contiguous(section_type start, section_type end) {
        end = static_cast<section_type>(end + 1);
        std::vector<std::byte> all_data;
        for (section_type i = start; i != end;
            i = static_cast<section_type>(i + 1)
        ) {
            const auto& section_span = get_section_by_id(static_cast<section_type>(i % num_sections)).data_span();
            std::copy(section_span.begin(), section_span.end(), std::back_inserter(all_data));
        }
        return all_data;
    }
};
static_assert(sizeof(game_save) == 0xE000);

struct mystery_gift_wonder_card {
    uint16_t checksum;
    uint16_t padding;
    union {
        struct {
            uint16_t event_id;
            uint16_t default_icon;
            uint32_t count;
            uint8_t flag;
            uint8_t stamp_max;
            std::array<uint8_t, 40> title;
            std::array<uint8_t, 40> subtitle;
            std::array<uint8_t, 40> contents_line_1;
            std::array<uint8_t, 40> contents_line_2;
            std::array<uint8_t, 40> contents_line_3;
            std::array<uint8_t, 40> contents_line_4;
            std::array<uint8_t, 40> warning_line_1;
            std::array<uint8_t, 40> warning_line_2;
        };
        std::array<std::byte, 332> data;
    };
    std::array<std::byte, 10> padding0;
    uint16_t icon;
    void check() {
        check_m(crc16(std::span(data)) == checksum);
    }
};

static_assert(sizeof(mystery_gift_wonder_card) == 348);

struct mystery_gift_event_script {
    uint16_t checksum;
    uint16_t padding;
    std::array<std::byte, 1000> event_script;
    void check() {
        check_m(crc16(this->event_script) == checksum);
    }
};

static_assert(sizeof(mystery_gift_event_script) == 1004);

struct mystery_gift_file_format {
    mystery_gift_wonder_card wonder_card;
    std::array<std::byte, 68> padding1;
    mystery_gift_event_script event_script;

    void check() {
        wonder_card.check();
        event_script.check();
    }
};

static_assert(offsetof(mystery_gift_file_format, wonder_card) == 0);
static_assert(offsetof(mystery_gift_file_format, event_script) == 416);

struct mystery_gift_save_format_emerald {
    std::array<std::byte, 1388> padding0;
    mystery_gift_wonder_card wonder_card;
    std::array<std::byte, 480> padding1;
    mystery_gift_event_script event_script;
};

static_assert(offsetof(mystery_gift_save_format_emerald, wonder_card) == 1388);
static_assert(offsetof(mystery_gift_save_format_emerald, event_script) == 2216);

struct mystery_gift_save_format_frlg {
    std::array<std::byte, 1120> padding0;
    mystery_gift_wonder_card wonder_card;
    std::array<std::byte, 480> padding1;
    mystery_gift_event_script event_script;
};

static_assert(offsetof(mystery_gift_save_format_frlg, wonder_card) == 1120);
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

const std::string_view species_name(uint16_t national_id) {
    const uint16_t n = national_id - 1;
    if (n < pokemon_names.size()) {
        return pokemon_names[n];
    } else {
        return "error";
    }
}

constexpr std::pair<uint16_t, uint16_t> gen_id_range(uint8_t gen) {
    if (gen == 1) {
        return {1, 151};
    } else if (gen == 2) {
        return {152, 251};
    } else if (gen == 3) {
        return {252, 386};
    } else {
        return {};
    }
}

constexpr uint8_t gen(uint16_t national_id) {
    if (national_id <= 151) {
        return 1;
    } else if (national_id <= 251) {
        return 2;
    } else if (national_id <= 386) {
        return 3;
    } else {
        throw "error";
    }
}

constexpr bool legendary(uint16_t national_id) {
    return
        (national_id >= 144 && national_id <= 146) || national_id == 150 ||
        (national_id >= 243 && national_id <= 245) || national_id == 249 || national_id == 250 ||
        (national_id >= 377 && national_id <= 384);
}

constexpr bool mythical(uint16_t national_id) {
    return
        national_id == 151 ||
        national_id == 251 ||
        national_id == 385 || national_id == 386;
}

constexpr bool starter(uint16_t national_id) {
    return
        (national_id >= 1 && national_id < 1 + 9) ||
        (national_id >= 152 && national_id < 152 + 9) ||
        (national_id >= 252 && national_id < 252 + 9);
}

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
        return personality == 0;
    }

    void decode() {
        uint8_t index = personality % 24;
        std::array<uint8_t, 4> order = pokemon_data_orders[index];
        for (uint8_t i = 0; i < 4; i++) {
            for (uint8_t j = i + 1; j < 4; j++) {
                if (order[i] < order[j]) {
                    continue;
                } else {
                    std::swap(data[i], data[j]);
                    std::swap(order[i], order[j]);
                }
            }
        }
        assert(std::is_sorted(order.begin(), order.end()));
        uint32_t decryption_key = original_trainer_id ^ personality;
        xor_bytes(span_bytes<pokemon_data_growth>(std::span(data)), decryption_key);
    }
    void check() {
        std::span<uint16_t, sizeof(data) / sizeof(uint16_t)> s{reinterpret_cast<uint16_t*>(&data), sizeof(data) / sizeof(uint16_t)};

        uint16_t sum = std::accumulate(s.begin(), s.end(), 0);
        if (sum != checksum) {
            std::cerr << "checksum mismatch! expected " << checksum << " got " << sum << std::endl;
        }
    }

    std::string nickname_str() const {
        return pokemon_string_to_string(nickname);
    }

    std::string original_trainer_name_str() const {
        return pokemon_string_to_string(original_trainer_name);
    }

    uint8_t level() const {
        return experience_to_level(national_id(), growth.experience);
    }

    bool shiny() const {
        uint32_t x = personality ^ original_trainer_id;
        uint16_t y = (x >> 16) ^ x;
        return y < 8;
    }

    uint8_t gen() const {
        return ::gen(national_id());
    }

    bool legendary() const {
        return ::legendary(national_id());
    }

    bool mythical() const {
        return ::mythical(national_id());
    }

    bool starter() const {
        return ::starter(national_id());
    }

    uint16_t national_id() const {
        return internal_to_national(growth.species);
    }

    const std::string_view species_name() const {
        return ::species_name(national_id());
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
    os << "level " << static_cast<int>(p.level) << " " << p.species_name();
    return os;
}

std::ostream& operator<<(std::ostream& os, const pokemon_box& p) {
    os << "level " << static_cast<int>(p.level()) << " " << p.species_name();
    return os;
}

struct sections_pc_buffer {
    uint32_t current_pc_buffer;
    std::array<pokemon_box, 420> pc_buffer_pokemon;
    std::array<std::array<char, 9>, 14> box_names;
    std::array<char, 14> box_wallpapers;
};

static_assert(sizeof(pokemon_party) == 100);
static_assert(sizeof(pokemon_box) == 80);

struct section_trainer_info: public section {
    enum game_version game_version() {
        uint32_t game_code = span_cast<uint32_t>(data_span().subspan(0xac, 4)).front();
        switch (game_code) {
            case 0x00000000:
                return game_version::ruby_sapphire;
            case 0x00000001:
                return game_version::leafgreen_firered;
            default:
                return game_version::emerald;
        }
    }

    bool pokedex_owned(uint16_t national_id) {
        auto i = national_id - 1;
        auto s = data_span().subspan(0x28, 49);
        bool owned = static_cast<bool>((s[i >> 3] >> (i & 7)) & std::byte{1});
        return owned;
    }
};

struct section_team_items: public section {
    std::span<pokemon_party> get_pokemon_party(game_version gv) {
        std::array team_size_offsets = {0x234, 0x34, 0x234};
        std::array team_list_offsets = {0x238, 0x38, 0x238};

        size_t team_size_offset = team_size_offsets[gv];
        size_t team_list_offset = team_list_offsets[gv];
        size_t team_size = span_cast<uint32_t>(data_span().subspan(team_size_offset, 4)).front();
        assert(team_size <= 6);
        auto team_list = span_cast<pokemon_party>(data_span().subspan(team_list_offset, team_size * sizeof(pokemon_party)));
        return team_list;
    }
};

struct pokemon_gen3_format {
    game_save a;
    game_save b;
    std::array<std::byte, 8192> hall_of_fame;
    std::array<std::byte, 4096> mystery_gift;
    std::array<std::byte, 4096> recorded_battle;

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

    enum game_version game_version() {
        auto trainer_info = static_cast<section_trainer_info>(get_latest_game_save().get_section_by_id(section_type::trainer_info));
        return trainer_info.game_version();
    }

    void write_mystery_gift(mystery_gift_file_format& mg) {
        section& s = get_latest_game_save().get_section_by_id(section_type::rival_info);
        if (game_version() == game_version::leafgreen_firered) {
            auto d = reinterpret_cast<mystery_gift_save_format_frlg*>(s.data.data());
            d->wonder_card = mg.wonder_card;
            d->event_script = mg.event_script;
        } else if (game_version() == game_version::emerald) {
            auto d = reinterpret_cast<mystery_gift_save_format_emerald*>(s.data.data());
            d->wonder_card = mg.wonder_card;
            d->event_script = mg.event_script;
        }
        s.checksum = s.calculate_checksum();
    }
};

static_assert(offsetof(pokemon_gen3_format, a) == 0);
static_assert(offsetof(pokemon_gen3_format, b) == 0xE000);
static_assert(sizeof(pokemon_gen3_format) == 128 * 1024);
