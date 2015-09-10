#ifndef INCLUDE_UTILS_H
#define INCLUDE_UTILS_H

#include "timetable_types.h"
#include "genetic/fitness.h"

#include <set>
#include <vector>
#include <memory>

namespace utils {
    /**
     * A centralized class for computing and storing statistics about a population.
     */
    class PopulationStatistics {
    private:
        static int global_index;

        /**
         * The Kahan summation algorithm for a stable sum of values.
         * https://en.wikipedia.org/wiki/Kahan_summation_algorithm
         */
        static double kahan_sum(std::vector<FitnessPair>& values);
    public:
        int index;
        double min;
        double max;
        double mean;
        double median;
        double lower_quartile;
        double upper_quartile;

        /**
         * Compute and return an object of statistics from a list of fitnesses.
         */
        static PopulationStatistics compute(std::vector<FitnessPair>& fitnesses, std::shared_ptr<FitnessCore>& fitness_core);

        void print();
    };

    int count_overlaps(std::set<timetable_student_t> v1, std::set<timetable_student_t> v2);
    int count_overlaps(std::set<timetable_professor_t> v1, std::set<timetable_professor_t> v2);

    /**
     * A template function that converts map values to a vector.
     */
    template <typename TMap, typename TVec>
    std::vector<TVec> map_to_vector(const TMap &m) {
        std::vector<TVec> res = std::vector<TVec>();
        for (typename TMap::const_iterator it = m.begin(); it != m.end(); it++) {
            res.push_back(it->second);
        }
        return res;
    }

    /**
     * Compares two sorted vectors. Returns true if same, false if not.
     */
    bool compare_sorted_vectors(std::shared_ptr<std::vector<int>>& v1, std::shared_ptr<std::vector<int>>& v2);

    /**
     * Packs the time into a single value for variance calculation.
     */
    int get_packed_slot_time(timetable_day_t day, timetable_hour_t hour, timetable_hour_t earliest_hour, timetable_hour_t latest_hour);

    unsigned int get_random_seed();
}

#endif //INCLUDE_UTILS_H
