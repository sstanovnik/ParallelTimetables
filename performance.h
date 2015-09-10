#ifndef INCLUDE_PERFORMANCE_H
#define INCLUDE_PERFORMANCE_H

#include <chrono>
#include <vector>
#include <string>

typedef std::chrono::time_point<std::chrono::high_resolution_clock> hirez_time_t;

/**
 * A class to measure performance.
 * Facilitates measuring time, storing those values and computing them in a batch.
 */
class PerformanceBenchmark {
private:
    hirez_time_t program_start;
    hirez_time_t program_end;

    hirez_time_t initial_generation_start;
    hirez_time_t initial_generation_end;

    hirez_time_t prerequisite_initialization_start;
    hirez_time_t prerequisite_initialization_end;

    std::vector<hirez_time_t> generation_starts;
    std::vector<hirez_time_t> generation_ends;

    std::vector<hirez_time_t> fitness_computation_starts;
    std::vector<hirez_time_t> fitness_computation_ends;

    std::vector<hirez_time_t> population_fitness_sending_starts;
    std::vector<hirez_time_t> population_fitness_sending_ends;

    std::vector<hirez_time_t> selection_starts;
    std::vector<hirez_time_t> selection_ends;

    std::vector<hirez_time_t> survivor_indices_broadcast_starts;
    std::vector<hirez_time_t> survivor_indices_broadcast_ends;

    std::vector<hirez_time_t> survivor_processing_starts;
    std::vector<hirez_time_t> survivor_processing_ends;

    std::vector<hirez_time_t> survivor_allgather_starts;
    std::vector<hirez_time_t> survivor_allgather_ends;

    std::vector<hirez_time_t> repopulation_starts;
    std::vector<hirez_time_t> repopulation_ends;

    std::vector<hirez_time_t> population_adjustment_starts;
    std::vector<hirez_time_t> population_adjustment_ends;

    static const std::string separator;

    /**
     * Prints data about a single time.
     */
    static void print_simple_time(const std::string tag, hirez_time_t& start, hirez_time_t& end);

    /**
     * Prints data about a series of times, including min, max and avg calculations.
     */
    static void print_complex_time(const std::string tag, std::vector<hirez_time_t>& starts, std::vector<hirez_time_t>& ends);

public:
    static const bool START = true;
    static const bool END   = false;

    static const int PROGRAM = 0;
    static const int INITIAL_GENERATION = 1;
    static const int PREREQ_INIT = 2;
    static const int GENERATION = 3;
    static const int FITNESS_COMPUTATION = 4;
    static const int POPULATION_FITNESS_SENDING = 5;
    static const int SELECTION = 6;
    static const int SURVIVOR_INDICES_BROADCAST = 7;
    static const int SURVIVOR_PROCESSING = 8;
    static const int SURVIVOR_ALLGATHER = 9;
    static const int REPOPULATION = 10;
    static const int POPULATION_ADJUSTMENT = 11;

    PerformanceBenchmark();

    void measure_time(int category, bool startend);

    void print_stats();

    double get_latest_generation_time();

    /**
     * Gets the time spent on dynamic workloads in the last round.
     * Dynamic content is the content that can be modified by modifying the node population size.
     */
    double get_latest_round_processing_time();
};


#endif //INCLUDE_PERFORMANCE_H
