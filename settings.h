#ifndef INCLUDE_SETTINGS_H
#define INCLUDE_SETTINGS_H

#include <boost/serialization/access.hpp>
#include <string>

class Settings {
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & this->population_size;
        ar & this->survivor_ratio;
        ar & this->rounds;
        ar & this->mutation_probability;
        ar & this->crossover_probability;
        ar & this->stats_round_divisor;
    }

public:
    int population_size;
    double survivor_ratio;
    int rounds;
    double mutation_probability;
    double crossover_probability;
    int stats_round_divisor;

    static Settings import_from_file(std::string file_path);

    void print_settings();
};

#endif //INCLUDE_SETTINGS_H
