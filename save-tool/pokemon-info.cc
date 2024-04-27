#include "mmap.hh"

#include <iostream>
#include <span>
#include <numeric>
#include <vector>
#include <string>
#include <cassert>

#include "pokemon-emerald-format.hh"
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
            auto f = span_cast<pokemon_emerald_format>(d).front();
            f.check();
            auto team_items_data = f.get_latest_game_save().get_section_by_id(section_type::team_items).data;
            auto game_version = span_cast<section_trainer_info>(f.get_latest_game_save().get_section_by_id(section_type::trainer_info).data).front().game_version();
            std::cout << game_version << std::endl;
            size_t team_size_offset = team_size_offsets[game_version];
            size_t team_list_offset = team_list_offsets[game_version];
            size_t team_size = span_cast<uint32_t>(std::span(team_items_data).subspan(team_size_offset, 4)).front();
            assert(team_size <= 6);
            auto team_list = span_cast<pokemon>(std::span(team_items_data).subspan(team_list_offset, team_size * sizeof(pokemon)));
            for (auto& pokemon: team_list) {
                pokemon.decode();
                std::cout << pokemon << std::endl;
            }
        } catch (const std::runtime_error& e) {
            std::cout << "error in " << filename << ": " << e.what() << std::endl;
        }
    }
}
