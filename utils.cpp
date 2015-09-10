#include "utils.h"

#include <iomanip>
#include <chrono>

int utils::PopulationStatistics::global_index = 0;

double utils::PopulationStatistics::kahan_sum(std::vector<FitnessPair>& values) {
    double sum = 0;
    double c = 0;
    for (FitnessPair& v_full : values) {
        double v = v_full.fitness;
        double y = v - c;
        double t = sum + y;

        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}

utils::PopulationStatistics utils::PopulationStatistics::compute(std::vector<FitnessPair>& fitnesses, std::shared_ptr<FitnessCore>& fitness_core) {
    utils::PopulationStatistics::global_index++;
    utils::PopulationStatistics result = utils::PopulationStatistics();

    // median and quartile calculation depends on the data being sorted
    std::sort(fitnesses.begin(), fitnesses.end(), [](FitnessPair& a, FitnessPair&b) { return a.fitness < b.fitness; });

    unsigned int num_elements = (unsigned int) fitnesses.size();
    result.min = fitnesses.front().fitness;
    result.max = fitnesses.back().fitness;
    result.mean = kahan_sum(fitnesses) / num_elements;
    result.median = num_elements % 2 == 0 ? ((fitnesses[num_elements / 2].fitness + fitnesses[num_elements / 2 + 1].fitness) / 2.0) : (fitnesses[num_elements / 2].fitness);
    result.lower_quartile = fitnesses[num_elements / 4].fitness;
    result.upper_quartile = fitnesses[(3 * num_elements) / 4].fitness;

    return result;
}

void utils::PopulationStatistics::print() {
    std::cout << std::setprecision(5) << std::fixed;
    std::cout << "STATISTICS: " << std::endl;
    std::cout << "\t" << "min" << ": \t"  << this->min << std::endl;
    std::cout << "\t" << "max" << ": \t"  << this->max << std::endl;
    std::cout << "\t" << "mean" << ": \t"  << this->mean << std::endl;
    std::cout << "\t" << "median" << ": "  << this->median << std::endl;
    std::cout << "\t" << "lower quartile" << ": "  << this->lower_quartile << std::endl;
    std::cout << "\t" << "upper quartile" << ": "  << this->upper_quartile << std::endl;

    std::cout << std::flush;
}

int utils::count_overlaps(std::set<timetable_student_t> v1, std::set<timetable_student_t> v2) {
    int count = 0;
    for (auto i1 = v1.begin(); i1 != v1.end(); i1++) {
        for (auto i2 = v2.begin(); i2 != v2.end(); i2++) {
            if ((*i1) == (*i2)) {
                count++;
            }
        }
    }

    return count;
}

int utils::count_overlaps(std::set<timetable_professor_t> v1, std::set<timetable_professor_t> v2) {
    int count = 0;
    for (auto i1 = v1.begin(); i1 != v1.end(); i1++) {
        for (auto i2 = v2.begin(); i2 != v2.end(); i2++) {
            if ((*i1) == (*i2)) {
                count++;
            }
        }
    }

    return count;
}

bool utils::compare_sorted_vectors(std::shared_ptr<std::vector<int>>& v1, std::shared_ptr<std::vector<int>>& v2) {
    if (v1->size() != v2->size()) {
        return false;
    }

    for (size_t i = 0; i < v1->size(); i++) {
        if ((*v1)[i] != (*v2)[i]) {
            return false;
        }
    }

    return true;
}

int utils::get_packed_slot_time(timetable_day_t day, timetable_hour_t hour, timetable_hour_t earliest_hour, timetable_hour_t latest_hour) {
    return (day * (latest_hour - earliest_hour)) + (hour - earliest_hour);
}

unsigned int utils::get_random_seed() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    return (unsigned int) (std::chrono::high_resolution_clock::now().time_since_epoch().count() + (rank * 42));
}
