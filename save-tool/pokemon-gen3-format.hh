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
uint16_t crc16(std::span<uint8_t> data) {
    uint16_t v2 = 0x1121;
    for (auto x: data) {
        v2 = crc16_ccitt_table_16[(v2 ^ x) % 256] ^ (v2 >> 8);
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

    bool shiny() const {
        uint16_t personality_high = (personality >> 16) & 0xffff;
        uint16_t personality_low = personality & 0xffff;
        uint16_t secret_id = (original_trainer_id >> 16) & 0xffff;
        uint16_t s = original_trainer_id ^ secret_id ^ personality_high ^ personality_low;
        return s < 8;
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
    os << p.species_name();
    return os;
}

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

    enum game_version game_version() {
        uint8_t game_id = static_cast<uint8_t>(get_latest_game_save().get_section_by_id(section_type{0}).data[0xac]);
        switch (game_id) {
            case 0x00:
                return game_version::ruby_sapphire;
            case 0x01:
                return game_version::leafgreen_firered;
            default:
                return game_version::emerald;
        }
    }

    void write_mystery_gift(mystery_gift_file_format& mg) {
        section& s = get_latest_game_save().get_section_by_id(section_type{4});
        if (game_version() == game_version::leafgreen_firered) {
            auto d = reinterpret_cast<mystery_gift_save_format_frlg*>(s.data.data());
            d->wonder_card = mg.wonder_card;
            d->icon = mg.icon;
            d->event_script = mg.event_script;
        } else if (game_version() == game_version::emerald) {
            auto d = reinterpret_cast<mystery_gift_save_format_emerald*>(s.data.data());
            d->wonder_card = mg.wonder_card;
            d->icon = mg.icon;
            d->event_script = mg.event_script;
        }
        s.checksum = s.calculate_checksum();
    }
};

static_assert(offsetof(pokemon_gen3_format, a) == 0);
static_assert(offsetof(pokemon_gen3_format, b) == 0xE000);
static_assert(sizeof(pokemon_gen3_format) == 128 * 1024);
