#ifndef INCLUDE_TIMETABLE_H
#define INCLUDE_TIMETABLE_H

#include "timetable_types.h"
#include "import.h"
#include "genetic/fitness.h"

#include <boost/serialization/set.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include <set>
#include <vector>
#include <memory>
#include <random>

//////////////////////
// TYPE DEFINITIONS //
//////////////////////

class TimetableEntry {
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & this->day;
        ar & this->hour;
        ar & this->subject;
        ar & this->lectures;
        ar & this->classroom;
        ar & this->students;
        ar & this->professors;
    }

public:
    TimetableEntry();
    timetable_day_t day;    // 0 - 6 where 0 is monday
    timetable_hour_t hour;   // 0 - 23 where 0 is midnight
    timetable_subject_t subject;
    bool lectures; // true: lectures (3h per week), false: tutorials (2h per week)
    timetable_classroom_t classroom;
    std::set<timetable_student_t> students;
    std::set<timetable_professor_t> professors;

    /**
     * Compares two timetable entries: sorting by subject, then by whether these are lectures,
     * then by classroom, then by time. This is required for crossover alignment.
     * Returns true if a should appear before b.
     */
    static bool compare_subject_lectures_classroom_time(const std::shared_ptr<TimetableEntry>& a,
                                                        const std::shared_ptr<TimetableEntry>& b);

    static bool compare_time(const std::shared_ptr<TimetableEntry>& a,
                             const std::shared_ptr<TimetableEntry>& b);

    /**
     * Clones this object and creates a new standalone instance. Deep copy.
     */
    std::shared_ptr<TimetableEntry> clone();

    /**
     * Whether the two entries match each other as lectures - they are within range of each other.
     * Does not return true when comparing with oneself.
     */
    bool is_matching_lecture(std::shared_ptr<TimetableEntry>& other);

    /**
     * Similar to the above, but limits to neighbouring lectures (no gap).
     */
    bool is_matching_lecture_strict(std::shared_ptr<TimetableEntry>& other);

    /**
     * Whether the two entries match each other as tutorials - they are the same double cycle.
     * Does not return true when comparing with oneself.
     */
    bool is_matching_tutorial(std::shared_ptr<TimetableEntry>& other);

    void print();
};

class Timetable {
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & this->timetable_entries;
    }

public:
    /**
     * A performance optimization, essentially a dirty bit for whether this is sorted.
     */
    bool sorted;

    // timetable entries sorted by subject, then by time (efficiency, other sorting orders as needed)
    std::vector<std::shared_ptr<TimetableEntry>> timetable_entries;

    Timetable();

    /**
     * Clones this object and creates a new standalone instance. Deep copy.
     * Actually only copies all timetable entries, no computed properties.
     * Only clones basic references, not the sorted ones.
     */
    std::shared_ptr<Timetable> clone();

    /**
     * Sorts the timetable. Currently there is only one sorting order - the default.
     */
    void sort();

    void print();

    /**
     * Serialize the JSON object to a file.
     */
    void export_json(std::string file_path);

    /**
     * Validates students in this timetable.
     * Used to figure out what is causing IDs to be brokd.
     * Returns 0 if valid, the ID of the offending student if invalid.
     */
    timetable_student_t validate_students(timetable_student_t max_student_id);
};


/**
 * Used for generating timetable individuals.
 * Exists because there is a persistent random state and some shared precomputation with the generation procedure.
 */
class TimetableGenerator {
private:
    std::vector<import::Professor> professor_list;
    std::vector<import::Classroom> classroom_list;
    std::vector<import::Student> student_list;
    std::vector<import::Subject> subject_list;

    std::mt19937 rand;
    std::uniform_int_distribution<timetable_day_t> day_distribution;
    std::uniform_int_distribution<timetable_hour_t> contiguous_hour_distribution_lectures;
    std::uniform_int_distribution<timetable_hour_t> contiguous_hour_distribution_tutorials;
public:
    TimetableGenerator(std::map<int, import::Professor>& professors,
                       std::map<int, import::Classroom>& classrooms,
                       std::map<int, import::Student>& students,
                       std::map<int, import::Subject>& subjects);

    std::shared_ptr<Timetable> generate();
};

#endif //INCLUDE_TIMETABLE_H
