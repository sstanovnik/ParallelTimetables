#ifndef INCLUDE_MUTATION_H
#define INCLUDE_MUTATION_H

#include "../import.h"
#include "../timetable.h"

#include <random>

class MutationCore {
private:
    // mutation constraints
    int min_hour;
    int max_hour;
    int min_day;
    int max_day;
    std::shared_ptr<std::vector<import::Subject>> imported_subjects;
    std::map<timetable_subject_t, std::vector<timetable_classroom_t>> subject_lecture_classrooms;
    std::map<timetable_subject_t, std::vector<timetable_classroom_t>> subject_tutorial_classrooms;

    // utilities
    std::mt19937 rand;
    std::uniform_int_distribution<int> mutation_point_distribution;
    std::uniform_int_distribution<timetable_day_t> day_distribution;
    std::uniform_int_distribution<timetable_hour_t> hour_distribution;
    std::uniform_real_distribution<double> zero_one_distribution;

    // one for each to avoid instantiating a different one each time (as lightweight as it's supposed to be)
    // http://stackoverflow.com/questions/19036141/vary-range-of-uniform-int-distribution
    std::map<timetable_subject_t, std::uniform_int_distribution<timetable_classroom_t>> subject_lecture_classroom_distributions;
    std::map<timetable_subject_t, std::uniform_int_distribution<timetable_classroom_t>> subject_tutorial_classroom_distributions;

    /**
     * Get a random lecture or tutorial classroom for the subject.
     */
    inline timetable_classroom_t get_random_lecture_classroom(timetable_subject_t subject_id);
    inline timetable_classroom_t get_random_tutorial_classroom(timetable_subject_t subject_id);

public:
    MutationCore(timetable_hour_t min_hour, timetable_hour_t max_hour, timetable_day_t min_day, timetable_day_t max_day, std::vector<import::Subject>& imported_subjects);

    /**
     * Performs a random mutation operation and returns a new object.
     * Will perform a mutation if no error is found. If an error is found, returns nullptr.
     */
    std::shared_ptr<Timetable> perform_mutation(std::shared_ptr<Timetable>& parent);
};


#endif //INCLUDE_MUTATION_H
