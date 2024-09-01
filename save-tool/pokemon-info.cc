#include "mmap.hh"

#include <iostream>
#include <span>
#include <numeric>
#include <vector>
#include <string>
#include <cassert>

#include "pokemon-gen3-format.hh"
#include "util.hh"

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
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
                auto team_items_section = static_cast<section_team_items>(save.get_section_by_id(section_type::team_items));
                auto game_version = f.game_version();
                auto party_pokemon = team_items_section.get_pokemon_party(game_version);
                std::cout << "party:" << std::endl;
                for (auto& pokemon: party_pokemon) {
                    pokemon.decode();
                    pokemon.check();
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
                    std::cout << pokemon << std::endl;
                }
            }
        } catch (const std::runtime_error& e) {
            std::cout << "error in " << filename << ": " << e.what() << std::endl;
        }
    }
}
