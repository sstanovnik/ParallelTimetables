#include "performance.h"

#include <iostream>
#include <assert.h>
#include <iomanip>
#include <math.h>

const std::string PerformanceBenchmark::separator = "    ";

PerformanceBenchmark::PerformanceBenchmark() {

}

void PerformanceBenchmark::measure_time(int category, bool startend) {
    hirez_time_t time = std::chrono::high_resolution_clock::now();

    if (startend == START) {
        switch (category) {
            case PROGRAM:
                this->program_start = time;
                break;
            case INITIAL_GENERATION:
                this->initial_generation_start = time;
                break;
            case PREREQ_INIT:
                this->prerequisite_initialization_start = time;
                break;
            case GENERATION:
                this->generation_starts.push_back(time);
                break;
            case FITNESS_COMPUTATION:
                this->fitness_computation_starts.push_back(time);
                break;
            case POPULATION_FITNESS_SENDING:
                this->population_fitness_sending_starts.push_back(time);
                break;
            case SELECTION:
                this->selection_starts.push_back(time);
                break;
            case SURVIVOR_INDICES_BROADCAST:
                this->survivor_indices_broadcast_starts.push_back(time);
                break;
            case SURVIVOR_PROCESSING:
                this->survivor_processing_starts.push_back(time);
                break;
            case SURVIVOR_ALLGATHER:
                this->survivor_allgather_starts.push_back(time);
                break;
            case REPOPULATION:
                this->repopulation_starts.push_back(time);
                break;
            case POPULATION_ADJUSTMENT:
                this->population_adjustment_starts.push_back(time);
                break;
            default:
                std::cerr << "Invalid measurement category. " << std::endl;
                throw std::exception();
        }
    } else {
        switch (category) {
            case PROGRAM:
                this->program_end = time;
                break;
            case INITIAL_GENERATION:
                this->initial_generation_end = time;
                break;
            case PREREQ_INIT:
                this->prerequisite_initialization_end = time;
                break;
            case GENERATION:
                this->generation_ends.push_back(time);
                break;
            case FITNESS_COMPUTATION:
                this->fitness_computation_ends.push_back(time);
                break;
            case POPULATION_FITNESS_SENDING:
                this->population_fitness_sending_ends.push_back(time);
                break;
            case SELECTION:
                this->selection_ends.push_back(time);
                break;
            case SURVIVOR_INDICES_BROADCAST:
                this->survivor_indices_broadcast_ends.push_back(time);
                break;
            case SURVIVOR_PROCESSING:
                this->survivor_processing_ends.push_back(time);
                break;
            case SURVIVOR_ALLGATHER:
                this->survivor_allgather_ends.push_back(time);
                break;
            case REPOPULATION:
                this->repopulation_ends.push_back(time);
                break;
            case POPULATION_ADJUSTMENT:
                this->population_adjustment_ends.push_back(time);
                break;
            default:
                std::cerr << "Invalid measurement category. " << std::endl;
                throw std::exception();
        }
    }
}

void PerformanceBenchmark::print_simple_time(const std::string tag, hirez_time_t &start, hirez_time_t& end) {
    unsigned long len = tag.length();
    const int target_start = 34;
    int spaces_to_insert = (int) (target_start - len);

    std::chrono::duration<double> total_time = end - start;
    std::cout << PerformanceBenchmark::separator << tag << ":";
    for (int i = 0; i < spaces_to_insert; i++) {
        std::cout << " ";
    }
    std::cout << total_time.count() << " s" << std::endl;
}

void PerformanceBenchmark::print_complex_time(const std::string tag, std::vector<hirez_time_t>& starts, std::vector<hirez_time_t>& ends) {
    assert(starts.size() == ends.size());

    unsigned long len = tag.length();
    const int target_start = 34;
    int spaces_to_insert = (int) (target_start - len);

    std::chrono::duration<double> min = ends[0] - starts[0];
    std::chrono::duration<double> max = min;
    std::chrono::duration<double> avg = min;
    for (size_t i = 0; i < starts.size(); i++) {
        std::chrono::duration<double> now = ends[i] - starts[i];

        if (now < min) min = now;
        if (now > max) max = now;

        avg += now;
    }

    avg /= starts.size();

    std::cout << PerformanceBenchmark::separator << tag << ":";
    for (int i = 0; i < spaces_to_insert; i++) {
        std::cout << " ";
    }
    std::cout << "min " << min.count() << " s; avg " << avg.count() << " s; max " << max.count() << " s" << std::endl;
}

void PerformanceBenchmark::print_stats() {
    std::cout << "Performance benchmark: " << std::setprecision(5) << std::endl;

    print_simple_time("Total time taken", program_start, program_end);
    print_simple_time("Initial generation", initial_generation_start, initial_generation_end);
    print_simple_time("Prerequisite initialization", prerequisite_initialization_start, prerequisite_initialization_end);

    print_complex_time("Complete generation", generation_starts, generation_ends);
    print_complex_time("Fitness computation", fitness_computation_starts, fitness_computation_ends);
    print_complex_time("Population fitness sending", population_fitness_sending_starts, population_fitness_sending_ends);
    print_complex_time("Selection", selection_starts, selection_ends);
    print_complex_time("Survivor indices broadcast", survivor_indices_broadcast_starts, survivor_indices_broadcast_ends);
    print_complex_time("Survivor processing", survivor_processing_starts, survivor_processing_ends);
    print_complex_time("Survivor allgather", survivor_allgather_starts, survivor_allgather_ends);
    print_complex_time("Repopulation", repopulation_starts, repopulation_ends);
    print_complex_time("Population adjustment", population_adjustment_starts, population_adjustment_ends);
}

double PerformanceBenchmark::get_latest_generation_time() {
    assert(generation_starts.size() == generation_ends.size());

    if (generation_starts.size() == 0) {
        std::chrono::duration<double> dur = initial_generation_end - initial_generation_start;
        return dur.count();
    }

    std::chrono::duration<double> dur = generation_ends.back() - generation_starts.back();
    return dur.count();
}

double PerformanceBenchmark::get_latest_round_processing_time() {
    std::chrono::duration<double> fitcomp = fitness_computation_ends.back() - fitness_computation_starts.back();
    std::chrono::duration<double> surproc = survivor_processing_ends.back() - survivor_processing_starts.back();
    std::chrono::duration<double> repop = repopulation_ends.back() - repopulation_starts.back();
    return fitcomp.count() + surproc.count() + repop.count();
}
