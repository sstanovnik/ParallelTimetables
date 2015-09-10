#include "import.h"
#include "utils.h"
#include "genetic/mutation.h"
#include "genetic/selection.h"
#include "genetic/crossover.h"
#include "custom_mpi.h"
#include "performance.h"
#include "settings.h"

#include <boost/math/common_factor.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/mpi.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
#include <boost/serialization/map.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <cmath>


#define MPI_MASTER 0

#define TRUE 1
#define FALSE 0

// output definitions - these can be overridden by setting them as compiler options
// print information about the variables used by the algorithm
#ifndef DEBUG_MODE
#define DEBUG_MODE FALSE
#endif

// print information about the progress of the program (generations are always printed)
#ifndef TRACE_MODE
#define TRACE_MODE FALSE
#endif

// print information about specific rounds, specified in the settings
#ifndef ROUND_STATS
#define ROUND_STATS TRUE
#endif

int main(int argc, char **argv) {
    boost::mpi::environment environment(argc, argv);
    boost::mpi::communicator world;

    PerformanceBenchmark bench = PerformanceBenchmark();
    bench.measure_time(PerformanceBenchmark::PROGRAM, PerformanceBenchmark::START);

    std::string processor_name = environment.processor_name();
    int size = world.size();
    int rank = world.rank();

    std::map<int, import::Professor> professors;
    std::map<int, import::Classroom> classrooms;
    std::map<int, import::Student> students;
    std::map<int, import::Subject> subjects;

    Settings settings;
    if (rank == MPI_MASTER) {
        settings = Settings::import_from_file("../xml/settings.xml");
#if TRACE_MODE
        std::cout << "Master finished parsing settings. " << std::endl;
#endif
#if DEBUG_MODE
        settings.print_settings();
#endif
        std::string professors_file = "../gen/professors.xml";
        std::string classrooms_file = "../gen/classrooms.xml";
        std::string students_file = "../gen/students.xml";
        std::string subjects_file = "../gen/subjects.xml";
        professors = import::Professor::import_professors(professors_file);
        classrooms = import::Classroom::import_classrooms(classrooms_file);
        students = import::Student::import_students(students_file);
        subjects = import::Subject::import_subjects(subjects_file);
#if TRACE_MODE
        std::cout << "Master finished parsing input files. " << std::endl;
#endif
    }


    // now that everything is parsed, broadcast it to everyone
    boost::mpi::broadcast(world, settings, MPI_MASTER);
#if TRACE_MODE
    std::cout << "Process " << rank << " got settings. " << std::endl;
#endif
    boost::mpi::broadcast(world, professors, MPI_MASTER);
#if TRACE_MODE
    std::cout << "Process " << rank << " got " << professors.size() << " professors. " << std::endl;
#endif
    boost::mpi::broadcast(world, classrooms, MPI_MASTER);
#if TRACE_MODE
    std::cout << "Process " << rank << " got " << classrooms.size() << " classrooms. " << std::endl;
#endif
    boost::mpi::broadcast(world, students, MPI_MASTER);
#if TRACE_MODE
    std::cout << "Process " << rank << " got " << students.size() << " students. " << std::endl;
#endif
    boost::mpi::broadcast(world, subjects, MPI_MASTER);
#if TRACE_MODE
    std::cout << "Process " << rank << " got " << subjects.size() << " subjects. " << std::endl;
#endif

    std::vector<import::Subject> subject_list = utils::map_to_vector<std::map<int, import::Subject>, import::Subject>(subjects);
#if TRACE_MODE
    std::cout << "Process " << rank << " parsed " << subject_list.size() << " subjects into a list. " << std::endl;
#endif


    // change the population size to be divisible by the number of nodes and the survivor count (but only increase it)
    // the logic behind this:
    //      assume size is the number we want the population size to be divisible with
    //      add the number of individuals that are left to the next divisible milestone
    //      the number of individuals that are over the previous milestone is (POPULATION_SIZE % size)
    //      we then subtract that from the total size to get the amount left, then
    //      mod it by size again so we handle cases where the population was already divisible (we'd add another whole size)
    // convoluted, but fun!
    int real_population_size = settings.population_size;
    int real_survivor_count = ((int) ceil(real_population_size * settings.survivor_ratio));
    int lcm = boost::math::lcm(size, real_survivor_count);
    real_population_size += (lcm - (settings.population_size % lcm)) % lcm; // make divisible
    int process_population_size = real_population_size / size;

#if DEBUG_MODE
    if (rank == MPI_MASTER) {
        std::cout << "The complete population of " << real_population_size << " (corrected) will be split into "
        << size << " parts of " << process_population_size << " individuals. We will have "
        << real_survivor_count << " total survivors in each generation. " <<  std::endl;
    }
#endif




#if TRACE_MODE
    std::cout << "Process " << rank << " started initial generation. " << std::endl;
#endif
    bench.measure_time(PerformanceBenchmark::INITIAL_GENERATION, PerformanceBenchmark::START);

    TimetableGenerator timetable_generator(professors, classrooms, students, subjects);
    std::vector<std::shared_ptr<Timetable>> process_population = std::vector<std::shared_ptr<Timetable>>();
    for (int i = 0; i < process_population_size; i++) {
        std::shared_ptr<Timetable> gend = timetable_generator.generate();
        process_population.push_back(gend);
        std::shared_ptr<Timetable>& timetable = process_population.back();

        // check students validity
        timetable_student_t possible_offender = gend->validate_students((timetable_student_t) (students.size() - 1));
        if (possible_offender != 0) {
            std::cerr << "Process " << rank << " generated an invalid timetable due to at least one invalid student (" << possible_offender << "). " << std::endl;
            environment.abort(-1);
            throw std::exception();
        }

        // debugging tool: check each timetable
        // each lectures entry must have its matching pair
        for (std::shared_ptr<TimetableEntry>& te : timetable->timetable_entries) {
            if (te->lectures) {
                continue;
            }

            bool found = false;
            for (std::shared_ptr<TimetableEntry>& te2 : timetable->timetable_entries) {
                if (te->is_matching_tutorial(te2)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                std::cerr << "Process " << rank << ": matching tutorial not found (post-generation check). " << std::endl;
                te->print();
                timetable->print();
                environment.abort(-1);
                throw std::exception();
            }
        }
    }

    bench.measure_time(PerformanceBenchmark::INITIAL_GENERATION, PerformanceBenchmark::END);

#if TRACE_MODE
    std::cout << "Process " << rank << " finished initial generation. " << std::endl;
#endif


    bench.measure_time(PerformanceBenchmark::PREREQ_INIT, PerformanceBenchmark::START);

    int round = 0;
    std::mt19937 rand = std::mt19937(utils::get_random_seed());
    std::uniform_int_distribution<int> survivor_selector(0, real_survivor_count - 1);
    std::uniform_real_distribution<double> zero_one_distribution(0, 1);
    MutationCore mut = MutationCore(EARLIEST_HOUR, LATEST_HOUR, 0, 4, subject_list);
    CrossoverCore cross = CrossoverCore(subject_list);
    std::shared_ptr<FitnessCore> fitness_core(new FitnessCore(professors, classrooms, students, subjects));
    TournamentSelection ts = TournamentSelection(real_survivor_count);

    std::vector<FitnessPair> process_population_fitnesses = std::vector<FitnessPair>((unsigned long) process_population_size);
    std::vector<FitnessPair> global_population_fitnesses = std::vector<FitnessPair>((unsigned long) real_population_size);
    std::vector<std::shared_ptr<Timetable>> global_survivors = std::vector<std::shared_ptr<Timetable>>();

    // dynamic workload management benchmarking
    int dynamic_workload_window_size = 3;
    double window_processing_time_sum = 0;
    std::vector<double> window_time_sums = std::vector<double>();
    std::vector<int> process_adjusted_population_counts = std::vector<int>();

    int process_individual_start_index = rank * process_population_size;
    int process_individual_end_index = process_individual_start_index + process_population_size; // exclusive

    // this is used to pad when sending fitnesses
    int max_process_population = process_population_size;

    bench.measure_time(PerformanceBenchmark::PREREQ_INIT, PerformanceBenchmark::END);

    do {
        if (rank == MPI_MASTER) {
            std::cout << std::setprecision(5) << "GENERATION " << round << " (" << bench.get_latest_generation_time() << " s)" << std::endl;
        }

        bench.measure_time(PerformanceBenchmark::GENERATION, PerformanceBenchmark::START);
        bench.measure_time(PerformanceBenchmark::FITNESS_COMPUTATION, PerformanceBenchmark::START);

        // compute the fitnesses of each individual in each process, storing them in a vector of pairs
        process_population_fitnesses.clear();
        int individual_index = process_individual_start_index;
        for (auto& i : process_population) {
            fitness_t fitness = fitness_core->calculate_fitness(i);
            FitnessPair fp;
            fp.individual_index = individual_index;
            fp.fitness = fitness.fitness;
            process_population_fitnesses.push_back(fp);

            individual_index++;
        }

#if DEBUG_MODE
        std::cout << "Pre-padding process " << rank << " population fitnesses size: " << process_population_fitnesses.size() << std::endl;
#endif

        // pad to send equal chunks (Boost MPI limitation)
        int remaining = (int) (max_process_population - process_population_fitnesses.size());
        for (int i = 0; i < remaining; i++) {
            FitnessPair fp = FitnessPair();
            fp.individual_index = -1;
            process_population_fitnesses.push_back(fp);
        }

#if DEBUG_MODE
        std::cout << "Post-padding process " << rank << " population fitnesses size: " << process_population_fitnesses.size()
                  << " (should be " << max_process_population << ")" << std::endl;
#endif

        bench.measure_time(PerformanceBenchmark::FITNESS_COMPUTATION, PerformanceBenchmark::END);
        bench.measure_time(PerformanceBenchmark::POPULATION_FITNESS_SENDING, PerformanceBenchmark::START);

        // send fitnesses to master
        if (rank == MPI_MASTER) {
            global_population_fitnesses.clear();
        }
        boost::mpi::gather(world, &process_population_fitnesses.front(), max_process_population, global_population_fitnesses, MPI_MASTER);

        bench.measure_time(PerformanceBenchmark::POPULATION_FITNESS_SENDING, PerformanceBenchmark::END);

//        if (rank == MPI_MASTER) {
//            std::sort(global_population_fitnesses.begin(), global_population_fitnesses.end(), [](FitnessPair& a, FitnessPair& b){return a.fitness > b.fitness;});
//            std::cout << "Process population fitnesses (idx): " << std::setprecision(1) << std::scientific;
//            for (auto& i : global_population_fitnesses) {
//                std::cout << i.individual_index << " (" << i.fitness << ") " << std::endl;
//            }
//            std::cout << std::endl;
//        }

        // filter out invalid (padded) fitnesses
        if (rank == MPI_MASTER) {
#if DEBUG_MODE
            std::cout << "Master removing paddings - before: " << global_population_fitnesses.size() << std::endl;
#endif
            // remove_if only moves to the end and returns the new end iterator
            auto new_end = std::remove_if(global_population_fitnesses.begin(),
                                          global_population_fitnesses.end(),
                                          [](FitnessPair& fp) { return fp.individual_index == -1; }
                           );
            global_population_fitnesses.erase(new_end, global_population_fitnesses.end());
#if DEBUG_MODE
            std::cout << "Master removing paddings - after: " << global_population_fitnesses.size()
                      << " (should be " << real_population_size << ")" << std::endl;
#endif
        }


#if ROUND_STATS
        // optional, every N rounds we print some stats
        if (round % settings.stats_round_divisor == 0 && rank == MPI_MASTER) {
            utils::PopulationStatistics stats = utils::PopulationStatistics::compute(global_population_fitnesses, fitness_core);
            stats.print();
        }
#endif

        bench.measure_time(PerformanceBenchmark::SELECTION, PerformanceBenchmark::START);

        // master performs selection
        std::vector<int> survivor_indices;
#if ROUND_STATS
        int best_individual_index = -1;
#endif
        if (rank == MPI_MASTER) {
#if DEBUG_MODE
            std::cout << "Master now has " << global_population_fitnesses.size() << " individuals. " << std::endl;
#endif
            survivor_indices = ts.perform_selection(global_population_fitnesses);

#if ROUND_STATS
            // get the individual with the absolute best fitness
            if (round % settings.stats_round_divisor == 0) {
                // this sorts descending -- best is first
                std::sort(global_population_fitnesses.begin(), global_population_fitnesses.end(), FitnessPair::compare_fitness);
                best_individual_index = global_population_fitnesses.front().individual_index;
            }
#endif

#if DEBUG_MODE
            std::cout << "indices: ";
            std::sort(survivor_indices.begin(), survivor_indices.end());
            for (int i : survivor_indices) {
                std::cout << i << " ";
            }
            std::cout << std::endl;
#endif
        }

#if ROUND_STATS
        // add the best individual index to the end to send with the other data
        survivor_indices.push_back(best_individual_index);
#endif

        bench.measure_time(PerformanceBenchmark::SELECTION, PerformanceBenchmark::END);
        bench.measure_time(PerformanceBenchmark::SURVIVOR_INDICES_BROADCAST, PerformanceBenchmark::START);

        // the survivors are broadcast to everyone
        boost::mpi::broadcast(world, survivor_indices, MPI_MASTER);

        bench.measure_time(PerformanceBenchmark::SURVIVOR_INDICES_BROADCAST, PerformanceBenchmark::END);

#if ROUND_STATS
        // get the stored best individual index and remove it from the survivor list to avoid pollution
        best_individual_index = survivor_indices.back();
        survivor_indices.pop_back();
#endif

#if DEBUG_MODE
        std::cout << "Process " << rank << " now has " << survivor_indices.size() << " survivor indices. " << std::endl;
#endif
        bench.measure_time(PerformanceBenchmark::SURVIVOR_PROCESSING, PerformanceBenchmark::START);

        // each process should now filter its survivors
        global_survivors.clear();

        std::set<int> survivor_indices_set = std::set<int>();
        survivor_indices_set.insert(survivor_indices.begin(), survivor_indices.end());
        int i = 0;
        int process_survivor_count = 0;
        std::vector<std::shared_ptr<Timetable>>::iterator it = process_population.begin();
        while (it != process_population.end()) {
#if ROUND_STATS
            // if we're printing stats this round and the individual is the best
            if (round % settings.stats_round_divisor == 0 && (process_individual_start_index + i) == best_individual_index) {
                // calculate fitness and print its details
                fitness_t f = fitness_core->calculate_fitness(*it);
                std::cout << "Process " << rank << " printing stats for index " << (process_individual_start_index + i) << std::endl;
                f.print_details();
            }
#endif

            // if the current individual's index survived
            if (survivor_indices_set.count(process_individual_start_index + i) == 1) {
                process_survivor_count++;
                it++;
            } else {
                it = process_population.erase(it);
            }
            i++;
        }

        bench.measure_time(PerformanceBenchmark::SURVIVOR_PROCESSING, PerformanceBenchmark::END);
#if DEBUG_MODE
        std::cout << "Process " << rank << " now has " << process_survivor_count << " survivors remaining. " << std::endl;
#endif
        bench.measure_time(PerformanceBenchmark::SURVIVOR_ALLGATHER, PerformanceBenchmark::START);

        // process_population variables now hold only survivors, but we must share them around
        // so all processes have all survivors
        custom_all_gather(world, process_population, global_survivors);

        bench.measure_time(PerformanceBenchmark::SURVIVOR_ALLGATHER, PerformanceBenchmark::END);
#if DEBUG_MODE
        std::cout << "Process " << rank << " now has (all) " << global_survivors.size() << " surviving individuals. Starting repopulation or population adjustment. " << std::endl;
#endif

        bench.measure_time(PerformanceBenchmark::POPULATION_ADJUSTMENT, PerformanceBenchmark::START);

        // at the end of the round, measure performance so we can adjust it dynamically
        if (round != 0) {
            window_processing_time_sum += bench.get_latest_round_processing_time();
        }

        // adjust performance every n rounds
        // round -1 and round > 1 to delay one round at the beginning
        if ((round - 1) % dynamic_workload_window_size == 0 && round > 1) {
            // send everything to master
            boost::mpi::gather(world, window_processing_time_sum, window_time_sums, MPI_MASTER);

            // master should now calculate the new counts
            if (rank == MPI_MASTER) {
                double time_sum = 0;
                for (double t : window_time_sums) {
                    time_sum += t;
                }

                double avg_time_ratio = 0;
                for (unsigned int process = 0; process < window_time_sums.size(); process++) {
                    avg_time_ratio += window_time_sums[process] / time_sum;
                }
                avg_time_ratio /= window_time_sums.size();

                int sum_sizes = 0;
                for (unsigned int process = 0; process < window_time_sums.size(); process++) {
                    // this is the ratio between this process' time and the total time
                    double process_time_ratio = window_time_sums[process] / time_sum;

                    // compute the difference in ratios
                    // if the average is larger than the time taken, increase the load
                    // otherwise decrease it
                    // this is done because simply using the processing time ratio would reset the values
                    // to worse because things would be more stabilized
                    double ratio_diff = avg_time_ratio - process_time_ratio;

                    // don't jump the gun
                    double ratio_adjustment = ratio_diff / 2;

                    double current_ratio = 1.0 * process_population_size / real_population_size;
                    double new_ratio = current_ratio + ratio_adjustment;

                    int adjusted_process_population = (int) boost::math::round(real_population_size * new_ratio);
                    process_adjusted_population_counts.push_back(adjusted_process_population);
                    sum_sizes += adjusted_process_population;
                }

                // fix the population size if needed
                // the size must be fixed because of some divisibility requirements at generation
                // and sending simplicity
                int population_size_difference = sum_sizes - real_population_size;
                if (population_size_difference > 0) {
#if TRACE_MODE
                    std::cout << "Master fixing population sizes due to a size difference of " << population_size_difference << std::endl;
#endif
                    int iii = 0;
                    while (population_size_difference > 0) {
                        process_adjusted_population_counts[iii % process_adjusted_population_counts.size()]--;
                        iii++;
                        population_size_difference--;
                    }
                } else if (population_size_difference < 0) {
#if TRACE_MODE
                    std::cout << "Master fixing population sizes due to a size difference of " << population_size_difference << std::endl;
#endif
                    int iii = 0;
                    while (population_size_difference < 0) {
                        process_adjusted_population_counts[iii % process_adjusted_population_counts.size()]++;
                        iii++;
                        population_size_difference++;
                    }
                }
            }

            // scatter new process sizes
            boost::mpi::broadcast(world, process_adjusted_population_counts, MPI_MASTER);

            // set the new max process population on each process
            max_process_population = 0;
            for (int ps : process_adjusted_population_counts) {
                if (ps > max_process_population) {
                    max_process_population = ps;
                }
            }

            // adjust the process size and figure out new bounds
            process_population_size = process_adjusted_population_counts[rank];
            process_individual_start_index = 0;
            for (int cpr = 0; cpr < rank; cpr++) {
                process_individual_start_index += process_adjusted_population_counts[cpr];
            }
            process_individual_end_index = process_individual_start_index + process_population_size;

            std::cout << "Population size adjusted for process " << rank << ": "
            << process_population_size << " (indices from " << process_individual_start_index
            << " to " << process_individual_end_index << ")" << std::endl;

            // clear these so we're clean
            process_adjusted_population_counts.clear();
            window_time_sums.clear();
        }

        bench.measure_time(PerformanceBenchmark::POPULATION_ADJUSTMENT, PerformanceBenchmark::END);
        bench.measure_time(PerformanceBenchmark::REPOPULATION, PerformanceBenchmark::START);

        // repopulate, so first clear the process population
        // (global_survivors holds the previous generation, this is the next one)
        process_population.clear();
        while (process_population.size() < (size_t) process_population_size) {
            int survivor_one = survivor_selector(rand);
            std::shared_ptr<Timetable>& selected_survivor = global_survivors[survivor_one];

            // don't always perform a mutation, sometimes just let the thing through
            double genetic_operator_random = zero_one_distribution(rand);
            if (genetic_operator_random < settings.crossover_probability) {
                // select an additional survivor
                int survivor_two;
                do {
                    survivor_two = survivor_selector(rand);
                } while (survivor_one == survivor_two);
                std::shared_ptr<Timetable>& selected_survivor_two = global_survivors[survivor_two];

                std::shared_ptr<Timetable> tt = cross.perform_crossover(selected_survivor, selected_survivor_two);
                process_population.push_back(tt);
            } else {
                std::shared_ptr<Timetable> tt = mut.perform_mutation(selected_survivor);

                // WORKAROUND: if there's an error (matching entry not found), skip and try again
                if (tt == nullptr) {
                    continue;
                }
                process_population.push_back(tt);
            }
        }

        bench.measure_time(PerformanceBenchmark::REPOPULATION, PerformanceBenchmark::END);
        bench.measure_time(PerformanceBenchmark::GENERATION, PerformanceBenchmark::END);

    } while (++round < settings.rounds);

    bench.measure_time(PerformanceBenchmark::PROGRAM, PerformanceBenchmark::END);

    // print stats in order with a bit of wait time so it's not jumbled up
    // this is to enable easier output parsing
    for (int i = 0; i < size; i++) {
        if (i == rank) {
            std::cout << "Process " << rank << ": Genetic algorithm finished!" << std::endl;
            bench.print_stats();
            std::cout << "Process " << rank << " stats end. " << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            world.barrier();
        } else {
            world.barrier();
        }
    }

    // each process gets the best timetable and then sends it to the master
    std::cout << "Process " << rank << " starting timetable export. " << std::endl;
    std::shared_ptr<Timetable> best = process_population.front();
    double best_fitness = fitness_core->calculate_fitness(process_population.front()).fitness;
    for (auto& i : process_population) {
        fitness_t fitness = fitness_core->calculate_fitness(i);

        if (fitness.fitness > best_fitness) {
            best_fitness = fitness.fitness;
            best = i;
        }
    }

#if DEBUG_MODE
    std::cout << "Process " << rank << "'s best timetable has fitness " << best_fitness << std::endl;
#endif

    // send
    std::vector<std::shared_ptr<Timetable>> best_timetables = std::vector<std::shared_ptr<Timetable>>();
    boost::mpi::gather(world, best, best_timetables, MPI_MASTER);

    // the master then gets the absolute best and serializes it
    if (rank == MPI_MASTER) {
        best = best_timetables.front();
        best_fitness = fitness_core->calculate_fitness(best_timetables.front()).fitness;
        for (auto& te : best_timetables) {
            fitness_t fitness = fitness_core->calculate_fitness(te);

            if (fitness.fitness > best_fitness) {
                best_fitness = fitness.fitness;
                best = te;
            }
        }

#if TRACE_MODE
        std::cout << "Master now exporting json timetable. " << std::endl;
#endif
        // now serialize the best entry
        best->export_json("timetable.json");
    }

    // a final sync so everyone exits at the same time
    world.barrier();

    return 0;
}
