#include "selection.h"

TournamentSelection::TournamentSelection(int expected_survivors) {
    this->expected_survivors = expected_survivors;

    this->rand = std::mt19937(utils::get_random_seed());
}

std::vector<int> TournamentSelection::perform_selection(std::vector<FitnessPair>& fitnesses) {
    unsigned long population_size = fitnesses.size();

    int tournament_size = (int) (population_size / expected_survivors);

    if (population_size % expected_survivors != 0) {
        std::cerr << "Population not a multiple of the the number of survivors. " << std::endl;
        throw std::exception();
    }

    std::vector<int> result = std::vector<int>();

    // shuffle the population: this way we get a random sample even with contiguous groups
    std::shuffle(fitnesses.begin(), fitnesses.end(), rand);

    // perform the selection
    // sort contiguous groups of tournament_size elements by their fitness,
    // then select the best individual and add it to the new population
    for (std::vector<FitnessPair>::iterator i = fitnesses.begin(); i != fitnesses.end(); i += tournament_size) {
        std::sort(i, i + tournament_size, FitnessPair::compare_fitness);

        // the first one survives
        // it is still valid, as only the contents below the iterator changed, the iterator itself was not sorted
        result.push_back(i->individual_index);
    }

    return result;
}
