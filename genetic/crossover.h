#ifndef INCLUDE_CROSSOVER_H
#define INCLUDE_CROSSOVER_H

#include "../timetable.h"

#include <random>

class CrossoverCore {
private:
    std::shared_ptr<std::vector<import::Subject>> imported_subjects;
    std::mt19937 rand;
    std::uniform_int_distribution<int> mutation_point_distribution;
    std::uniform_real_distribution<double> zero_one_distribution;

public:
    CrossoverCore(std::vector<import::Subject>& imported_subjects);

    /**
     * Performs a random crossover between the two timetables.
     * This is non-destructive and returns a new timetable.
     *
     * NOTE: Crossover requires the two timetables to be aligned. Subjects with differing amounts of entries
     * are handled, but the sorting function must sort so lectures and pairs of tutorials are aligned.
     */
    std::shared_ptr<Timetable> perform_crossover(std::shared_ptr<Timetable>& left,
                                                 std::shared_ptr<Timetable>& right);
};

#endif //INCLUDE_CROSSOVER_H
