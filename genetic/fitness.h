#ifndef INCLUDE_FITNESS_H
#define INCLUDE_FITNESS_H

#include "../import.h"
#include "../timetable.h"

#include <boost/serialization/access.hpp>
#include <boost/mpl/bool.hpp>

#include <memory>
#include <set>

// forward declaration
class Timetable;


#define EARLIEST_HOUR      7
#define LATEST_HOUR       19
#define SOFT_LATEST_HOUR  18
#define STUDENT_PREFERRED_START  8
#define STUDENT_PREFERRED_END   17


#define PROHIBITIVE_SCORE -99999

// prohibitives (this should never happen, so they have a low score)
#define START_TOO_EARLY_SCORE                   PROHIBITIVE_SCORE
#define END_TOO_LATE_SCORE                      PROHIBITIVE_SCORE
#define TIMETABLE_ENTRY_OVERLAP_SCORE           PROHIBITIVE_SCORE
#define TUTORIALS_NOT_DOUBLE_CYCLE_SCORE        PROHIBITIVE_SCORE
#define PROFESSOR_OVERLAP_SCORE                 PROHIBITIVE_SCORE
#define SUBJECT_LECTURE_OVERLAP_SCORE           PROHIBITIVE_SCORE
#define SUBJECT_LECTURE_TUTORIALS_OVERLAP_SCORE PROHIBITIVE_SCORE
#define PROFESSOR_OVER_LOAD_SCORE               PROHIBITIVE_SCORE
#define CLASSROOM_OVER_CAPACITY_SCORE           PROHIBITIVE_SCORE

// negatives (would rather they don't happen)
#define STUDENT_OVERLAP_SCORE      -30
#define SOFT_LATEST_HOUR_SCORE     -20
#define NON_ATTACHED_LECTURE_SCORE -50

// positives (increase score if conditions are met, but these are not required)
#define STUDENT_PREFERRED_START_BONUS 20
#define STUDENT_PREFERRED_END_BONUS   10
#define LECTURES_MERGED_BONUS         5
#define TUTORIALS_AFTER_LECTURES_BONUS 5

// this can be a positive or a negative, depending on the score
#define STUDENT_ENTRY_GROUPING_SCORE 20


/**
 * Stores the actual fitness value and score occurrence counts
 */
class fitness_t {
public:
    double fitness = 0;

    // counts
    int non_attached_lecture = 0;
    int start_too_early = 0;
    int end_too_late = 0;
    int end_too_late_soft = 0;
    int classroom_over_capacity = 0;
    int timetable_entry_overlap = 0;
    int professor_overlap = 0;
    int student_overlap = 0;
    int subject_lecture_tutorials_overlap = 0;
    int subject_lecture_overlap = 0;
    int tutorials_double_cycle = 0;
    int professor_over_load = 0;
    int student_preferred_start = 0;
    int student_preferred_end = 0;
    int lectures_merged = 0;
    int tutorials_after_lectures = 0;
    int student_entry_grouping_variance_smaller = 0;
    int student_entry_grouping_variance_larger = 0;

    inline void operator+=(double what);

    bool operator<(const fitness_t& other) const;

    void print_details();
};

class FitnessCore {
private:
    std::map<int, import::Professor> professors;
    std::map<int, import::Classroom> classrooms;
    std::map<int, import::Student> students;
    std::map<int, import::Subject> subjects;

    // computation utilities
    std::map<timetable_professor_t, unsigned int> professor_loads;

    // the pair is semantically <first: day, second: hour>
    std::map<int, std::pair<timetable_day_t, timetable_hour_t>> subject_lecture_ends;

    // this is a helper for checking tutorial pairness
    // the set contains a tutorial index (counter, local only) if it has a pair
    // this is because we check each pair only once for efficiency and entries that occur later in the list
    // do not have a pair, as we don't include any prior entries in the check
    std::set<int> tutorial_has_pair;

    std::map<timetable_student_t, bool> student_start_limit_conformities;
    std::map<timetable_student_t, bool> student_end_limit_conformities;

    std::map<timetable_student_t, std::vector<int>> student_entry_times;

    /**
     * Resets computation utilities in preparation for the next computation pass.
     */
    void reset_utilities();
public:
    FitnessCore(std::map<int, import::Professor>& professors,
                std::map<int, import::Classroom>& classrooms,
                std::map<int, import::Student>& students,
                std::map<int, import::Subject>& subjects);

    /**
     * Calculates the fitness of the specified individual.
     */
    fitness_t calculate_fitness(std::shared_ptr<Timetable>& timetable);
};

/**
 * A helper container for a pair of values.
 * This is transmitted to the master, who performs selection on them, then transmits them back.
 */
class FitnessPair {
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & this->individual_index;
        ar & this->fitness;
    }

public:
    int individual_index;
    double fitness;

    /**
     * Sorts the objects by fitness.
     * SORTS DESCENDING (best is first).
     */
    static bool compare_fitness(const FitnessPair& a, const FitnessPair& b);
};

/**
 * See the similar section in import.h
 */
namespace boost {
    namespace mpi {
        template <>
        struct is_mpi_datatype<FitnessPair> : boost::mpl::true_ { };
    }
}

#endif //INCLUDE_FITNESS_H
