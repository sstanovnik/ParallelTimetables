#include "settings.h"
#include "tinyxml2/tinyxml2.h"

#include <iostream>

Settings Settings::import_from_file(std::string file_path) {
    Settings result = Settings();

    tinyxml2::XMLDocument doc;
    doc.LoadFile(file_path.c_str());

    auto *root = doc.FirstChildElement();
    auto *population_size_el      = root->FirstChildElement();
    auto *survivor_ratio_el       = population_size_el->NextSiblingElement();
    auto *rounds_el               = survivor_ratio_el->NextSiblingElement();
    auto *mutation_probability_el = rounds_el->NextSiblingElement();
    auto *stats_round_divisor_el  = mutation_probability_el->NextSiblingElement();

    result.population_size = atoi(population_size_el->GetText());
    result.survivor_ratio  = atof(survivor_ratio_el->GetText());
    result.rounds          = atoi(rounds_el->GetText());
    result.mutation_probability  = atof(mutation_probability_el->GetText());
    result.crossover_probability = 1 - result.mutation_probability;
    result.stats_round_divisor   = atoi(stats_round_divisor_el->GetText());

    return result;
}

void Settings::print_settings() {
    std::cout << "SETTINGS: " << std::endl;
    std::cout << "    " << "Population size: " << this->population_size << std::endl;
    std::cout << "    " << "Survivor ratio:  " << this->survivor_ratio << std::endl;
    std::cout << "    " << "Rounds: " << this->rounds << std::endl;
    std::cout << "    " << "Mutation probability:  " << this->mutation_probability << std::endl;
    std::cout << "    " << "Crossover probability: " << this->crossover_probability << std::endl;
    std::cout << "    " << "Stats round divisor:   " << this->stats_round_divisor << std::endl;
}
