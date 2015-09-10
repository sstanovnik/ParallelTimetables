#ifndef INCLUDE_SELECTION_H
#define INCLUDE_SELECTION_H

#include "../timetable.h"
#include "../utils.h"
#include "fitness.h"

#include <memory>
#include <random>
#include <vector>

/**
 * An object that handles tournament selection.
 * Instantiated because of easier parameter and random state management.
 */
class TournamentSelection {
private:
    // the tournament size determines the size of each group and, in turn, the amount of surviving individuals
    int expected_survivors;

    // utilities
    std::mt19937 rand;

public:
    /**
     * Constructs the object that will perform all computation. It needs to have a state because of the random utilities.
     */
    TournamentSelection(int expected_survivors);

    /**
     * Performs selection on a map of index->fitness.
     * Returns a vector of survivor identifiers.
     */
    std::vector<int> perform_selection(std::vector<FitnessPair>& fitnesses);
};

#endif //INCLUDE_SELECTION_H
