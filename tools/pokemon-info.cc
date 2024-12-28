#include "mmap.hh"

#include <iostream>
#include <span>
#include <numeric>
#include <vector>
#include <string>
#include <bitset>
#include <cassert>

#include "pokemon-gen3-format.hh"
#include "util.hh"

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::bitset<gen_id_range(3).second + 1> dex;
    std::bitset<28> unowns{};
    for (auto& filename: args) {
        try {
            auto m = mmap_file(filename, false);
            auto d = m.data;
            if (d.size() != 32 * 4096) {
                throw std::runtime_error("wrong save file size");
            }
            auto f = span_cast<pokemon_gen3_format>(d).front();
            f.check();
            auto& save = f.get_latest_game_save();
            save.check();

            {
                auto trainer_info = static_cast<section_trainer_info>(save.get_section_by_id(section_type::trainer_info));
                for (uint16_t n = gen_id_range(1).first; n <= gen_id_range(3).second; n++) {
                    if (trainer_info.pokedex_owned(n)) {
                        dex.set(n);
                    }
                }
            }
            {
                auto team_items_section = static_cast<section_team_items>(save.get_section_by_id(section_type::team_items));
                auto game_version = f.game_version();
                auto party_pokemon = team_items_section.get_pokemon_party(game_version);
                std::cout << "party:" << std::endl;
                for (auto& pokemon: party_pokemon) {
                    pokemon.decode();
                    pokemon.check();
                    dex.set(pokemon.national_id());
                    const auto uf = pokemon.unown_form();
                    if (uf) {
                        unowns.set(*uf);
                    }
                    std::cout << pokemon << std::endl;
                }
            }

            {
                auto box_pokemon_data = save.get_sections_contiguous(section_type::pc_buffer_a, section_type::pc_buffer_i);
                auto& pc_buffer_pokemon = span_cast<sections_pc_buffer>(std::span(box_pokemon_data)).front().pc_buffer_pokemon;
                std::cout << "box:" << std::endl;
                for (auto& pokemon: pc_buffer_pokemon) {
                    if (pokemon.empty()) {
                        continue;
                    }
                    pokemon.decode();
                    pokemon.check();
                    dex.set(pokemon.national_id());
                    const auto uf = pokemon.unown_form();
                    if (uf) {
                        unowns.set(*uf);
                    }
                    std::cout << pokemon << std::endl;
                }
            }
        } catch (const std::runtime_error& e) {
            std::cout << "error in " << filename << ": " << e.what() << std::endl;
        }
    }
    std::cout << "all dex:   ";
    uint16_t size = gen_id_range(3).second - gen_id_range(1).first + 1;
    std::cout << dex.count() << " / " << size << " = ";
    std::cout << 100.0f * dex.count() / size << "%" << std::endl;
    for (uint8_t gen = 1; gen <= 3; gen++) {
        size_t count = 0;
        for (size_t i = gen_id_range(gen).first; i <= gen_id_range(gen).second; i++) {
            count += dex[i];
        }
        uint16_t size = gen_id_range(gen).second - gen_id_range(gen).first + 1;
        std::cout << "gen " << static_cast<int>(gen) << " dex: ";
        std::cout << count << " / " << size << " = ";
        std::cout << 100.0f * count / size << "%" << std::endl;
    }
    for (uint8_t gen = 1; gen <= 3; gen++) {
        std::cout << "gen " << static_cast<int>(gen) << " missing:" << std::endl;
        for (size_t i = gen_id_range(gen).first; i <= gen_id_range(gen).second; i++) {
            if (!dex[i]) {
                std::cout << species_name(i) << std::endl;
            }
        }
    }
    std::cout << "unown: " << unowns.count() << " / " << unowns.size() << " = " <<
        100.0f * unowns.count() / unowns.size() << "%" << std::endl;
}
