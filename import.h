#ifndef INCLUDE_IMPORT_H
#define INCLUDE_IMPORT_H

#include "timetable_types.h"

#include <boost/serialization/string.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/mpi/datatype.hpp>
#include <boost/mpl/bool.hpp>

#include <map>
#include <istream>
#include <vector>
#include <string>


namespace import {
    // this can be both a professor or a TA
    class Professor {
    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & this->id;
            ar & this->name;
            ar & this->available_hours;
        }

    public:
        // sourced from input
        timetable_professor_t id;
        std::string name;
        unsigned int available_hours; // the amount of hours this TA is available (ignored for those that are only professors)

        /**
         * Imports professors.
         * Format:
         *      <num_professors>
         *      <professor_id> <professor_name> <available_hours>
         *      ...
         */
        static std::map<int, Professor> import_professors(std::string& file_path);
    };


    class Classroom {
    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & this->id;
            ar & this->lecture_capacity;
            ar & this->tutorial_capacity;
        }

    public:
        // sourced from input
        timetable_classroom_t id;
        unsigned int lecture_capacity;
        unsigned int tutorial_capacity;

        /**
         * Imports classrooms.
         * Format:
         *      <num_classrooms>
         *      <classroom_id> <capacity>
         *      ...
         */
        static std::map<int, Classroom> import_classrooms(std::string& file_path);

        /**
         * Used for sorting with std::sort. Sorts by lecture capacity descending,
         * which also means tutorial capacities are sorted.
         */
        static struct {
            bool operator()(Classroom a, Classroom b) {
                return a.lecture_capacity > b.lecture_capacity;
            };
        } descending_capacity_sort;
    };


    class Student {
    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & this->id;
            ar & this->subjects;
        }

    public:
        // sourced from input
        timetable_student_t id;
        std::vector<timetable_subject_t> subjects;

        /**
         * Imports students.
         * Format:
         *      <num_students>
         *      <id>
         *      <num_subjects>
         *      <subjects ...>
         *      ...
         */
        static std::map<int, Student> import_students(std::string& file_path);
    };


    class Subject {
    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & this->id;
            ar & this->lecture_classrooms;
            ar & this->tutorial_classrooms;
            ar & this->professors;
            ar & this->teaching_assistants;
            ar & this->teaching_assistant_weights;
            ar & this->students;
        }

    public:
        // sourced from input
        timetable_subject_t id;
        std::vector<timetable_classroom_t> lecture_classrooms;
        std::vector<timetable_classroom_t> tutorial_classrooms;
        std::vector<timetable_professor_t> professors;
        std::vector<timetable_professor_t> teaching_assistants;
        std::vector<double> teaching_assistant_weights;

        // computed from a list of students
        std::vector<timetable_student_t> students;

        /**
         * Return a list of classroom IDs that fit this subject's requirements.
         */
        std::vector<Classroom> get_possible_classrooms(std::vector<Classroom>& classrooms, bool lectures);

        /**
         * Imports subjects.
         * Format:
         *      <num_subjects>
         *
         *      <id>
         *      <num_available_lecture_classrooms>
         *      <lecture_classroom ...>
         *      <num_available_tutorial_classroom>
         *      <tutorial_classroom>
         *      <num_professors>
         *      <professor ...>
         *      <num_assistants>
         *      <assistant> <choose_weight> // all weights should sum to 1
         *          ...
         */
        static std::map<int, Subject> import_subjects(std::string& file_path);

        /**
         * Given a list of students, populates this subject with the students that are taking it.
         */
        void populate_students(std::map<int, Student> student_map);
    };
}


/**
 * These are Boost's way of optimizing fixed structures. They must be placed outside of namespace import because C++.
 */
namespace boost {
    namespace mpi {
        template <>
        struct is_mpi_datatype<import::Professor> : boost::mpl::true_ { };

        template <>
        struct is_mpi_datatype<import::Classroom> : boost::mpl::true_ { };
    }
}

#endif //INCLUDE_IMPORT_H
