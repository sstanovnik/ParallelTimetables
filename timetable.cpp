#include "json/json.hpp"
#include "timetable.h"
#include "utils.h"

#include <fstream>
#include <sstream>

using json = nlohmann::json;

TimetableEntry::TimetableEntry() {
    this->students = std::set<timetable_student_t>();
    this->professors = std::set<timetable_professor_t>();
}

bool TimetableEntry::compare_subject_lectures_classroom_time(const std::shared_ptr<TimetableEntry>& a,
                                                             const std::shared_ptr<TimetableEntry>& b) {
    if (a->subject != b->subject) {
        return a->subject < b->subject;
    } else {
        if (a->lectures != b->lectures) {
            return a->lectures && !b->lectures;
        } else {
            if (a->classroom != b->classroom) {
                return a->classroom < b->classroom;
            } else {
                if (a->day != b->day) {
                    return a->day < b->day;
                } else {
                    return a->hour < b->hour;
                }
            }
        }
    }
}

bool TimetableEntry::compare_time(const std::shared_ptr<TimetableEntry> &a, const std::shared_ptr<TimetableEntry> &b) {
    if (a->day != b->day) {
        return a->day < b->day;
    } else {
        return a->hour < b->hour;
    }
}

std::shared_ptr<TimetableEntry> TimetableEntry::clone() {
    std::shared_ptr<TimetableEntry> result = std::shared_ptr<TimetableEntry>(new TimetableEntry());

    result->day = this->day;
    result->hour = this->hour;
    result->subject = this->subject;
    result->lectures = this->lectures;
    result->classroom = this->classroom;
    result->students = std::set<timetable_student_t>(this->students);
    result->professors = std::set<timetable_professor_t >(this->professors);

    return result;
}

bool TimetableEntry::is_matching_lecture(std::shared_ptr<TimetableEntry>& other) {
    return    this->lectures && other->lectures
           && this->subject == other->subject
           && this->day == other->day
           && this->hour != other->hour   // prevents comparing with oneself
           && abs(this->hour - other->hour) <= 2;
}

bool TimetableEntry::is_matching_lecture_strict(std::shared_ptr<TimetableEntry>& other) {
    return    this->lectures && other->lectures
           && this->subject == other->subject
           && this->day == other->day
           && this->hour != other->hour   // prevents comparing with oneself
           && abs(this->hour - other->hour) <= 1;
}

bool TimetableEntry::is_matching_tutorial(std::shared_ptr<TimetableEntry>& other) {
    return   !this->lectures && !other->lectures
           && this->subject == other->subject
           && this->day == other->day
           && this->hour != other->hour
           && this->classroom == other->classroom
           && abs(this->hour - other->hour) <= 1
           && this->students == other->students;
}

void TimetableEntry::print() {
    std::cout << "Timetable entry: " << std::endl
        << "\t" << "subject: " << ((int) this->subject) << std::endl
        << "\t" << "lectures: " << (this->lectures ? "true" : "false") << std::endl
        << "\t" << "day: " << ((int) this->day) << std::endl
        << "\t" << "hour: " << ((int) this->hour) << std::endl
        << "\t" << "classroom: " << ((int) this->classroom) << std::endl
        << "\t" << "students: ";

    for (int s : this->students) {
        std::cout << s << " ";
    }
    std::cout << std::endl;

    std::cout << "\t" << "professors: ";
    for (int p : this->professors) {
        std::cout << p << " ";
    }
    std::cout << std::endl;
}


Timetable::Timetable() {
    this->timetable_entries = std::vector<std::shared_ptr<TimetableEntry>>();
    this->sorted = false;
}

std::shared_ptr<Timetable> Timetable::clone() {
    std::shared_ptr<Timetable> result = std::shared_ptr<Timetable>(new Timetable());

    result->timetable_entries = std::vector<std::shared_ptr<TimetableEntry>>();
    for (std::shared_ptr<TimetableEntry>& te : this->timetable_entries) {
        result->timetable_entries.push_back(te->clone());
    }

    return result;
}

void Timetable::sort() {
    if (!this->sorted) {
        std::sort(this->timetable_entries.begin(), this->timetable_entries.end(), TimetableEntry::compare_subject_lectures_classroom_time);
        this->sorted = true;
    }
}

void Timetable::print() {
    std::cout << "###############" << std::endl;
    std::cout << "## TIMETABLE ##" << std::endl;
    std::cout << "###############" << std::endl;
    std::cout << "\tentries: " << this->timetable_entries.size() << std::endl;
    for (auto& te : this->timetable_entries) {
        te->print();
    }
    std::cout << std::flush;
}

TimetableGenerator::TimetableGenerator(std::map<int, import::Professor>& professors,
                                       std::map<int, import::Classroom>& classrooms,
                                       std::map<int, import::Student>& students,
                                       std::map<int, import::Subject>& subjects) {

    this->professor_list = utils::map_to_vector<std::map<int, import::Professor>, import::Professor>(professors);
    this->classroom_list = utils::map_to_vector<std::map<int, import::Classroom>, import::Classroom>(classrooms);
    this->student_list = utils::map_to_vector<std::map<int, import::Student>, import::Student>(students);
    this->subject_list = utils::map_to_vector<std::map<int, import::Subject>, import::Subject>(subjects);

    this->rand = std::mt19937(utils::get_random_seed());
    this->day_distribution = std::uniform_int_distribution<timetable_day_t>(0, 4);
    this->contiguous_hour_distribution_lectures = std::uniform_int_distribution<timetable_hour_t>(EARLIEST_HOUR, LATEST_HOUR - 2);
    this->contiguous_hour_distribution_tutorials = std::uniform_int_distribution<timetable_hour_t>(EARLIEST_HOUR, LATEST_HOUR - 1);

    // additional precomputation
    for (auto i = subject_list.begin(); i != subject_list.end(); i++) {
        i->populate_students(students);
    }
}

void Timetable::export_json(std::string file_path) {
    json result;
    json timetable_entries_array = json::array();
    for (std::shared_ptr<TimetableEntry>& te : timetable_entries) {
        json te_json;
        te_json["day"] = te->day;
        te_json["hour"] = te->hour;
        te_json["subject"] = te->subject;
        te_json["lectures"] = te->lectures;
        te_json["classroom"] = te->classroom;

        json students_array = json::array();
        for (timetable_student_t stud : te->students) {
            students_array.push_back(stud);
        }
        te_json["students"] = students_array;

        json professors_array = json::array();
        for (timetable_professor_t prof : te->professors) {
            professors_array.push_back(prof);
        }
        te_json["professors"] = professors_array;

        timetable_entries_array.push_back(te_json);
    }
    result["timetable_entries"] = timetable_entries_array;

    std::ofstream out_file;
    out_file.open(file_path);
    out_file << result;
    out_file.close();
}

timetable_student_t Timetable::validate_students(timetable_student_t max_student_id) {
    for (std::shared_ptr<TimetableEntry>& te : this->timetable_entries) {
        for (timetable_student_t stud : te->students) {
            if (stud > max_student_id) {
                return stud;
            }
        }
    }
    return 0;
}

std::shared_ptr<Timetable> TimetableGenerator::generate() {
    std::shared_ptr<Timetable> timetable = std::shared_ptr<Timetable>(new Timetable());

    // shuffle so we aren't biased by the import
    std::shuffle(this->professor_list.begin(), this->professor_list.end(), rand);
    std::shuffle(this->classroom_list.begin(), this->classroom_list.end(), rand);
    std::shuffle(this->student_list.begin(), this->student_list.end(), rand);
    std::shuffle(this->subject_list.begin(), this->subject_list.end(), rand);


    // generation
    for (auto s : this->subject_list) {
        // generate a lectures entry for each subject

        std::vector<import::Classroom> lecture_classrooms = s.get_possible_classrooms(this->classroom_list, true);
        std::vector<import::Classroom> tutorial_classrooms = s.get_possible_classrooms(this->classroom_list, false);

        std::uniform_int_distribution<timetable_classroom_t> lecture_classroom_index_distribution(0, (timetable_classroom_t) (lecture_classrooms.size() - 1));
        std::uniform_int_distribution<timetable_classroom_t> tutorial_classroom_index_distribution(0, (timetable_classroom_t) (tutorial_classrooms.size() - 1));
        std::uniform_int_distribution<timetable_professor_t> assistant_index_distribution(0, (timetable_professor_t) (s.teaching_assistants.size() - 1));


        timetable_day_t day = this->day_distribution(rand);
        timetable_hour_t start_hour = this->contiguous_hour_distribution_lectures(rand);
        timetable_classroom_t lec_clrm = lecture_classrooms[lecture_classroom_index_distribution(rand)].id;
        for (timetable_hour_t j = 0; j < 3; j++) {
            std::shared_ptr<TimetableEntry> te(new TimetableEntry());

            te->day = day;
            te->hour = start_hour + j;
            te->subject = s.id;
            te->lectures = true;
            te->classroom = lec_clrm;
            te->students.insert(s.students.begin(), s.students.end());
            te->professors.insert(s.professors.begin(), s.professors.end());

            timetable->timetable_entries.push_back(te);
        }

        // generate enough tutorial entries for each subject to cover all students
        int student_count = (int) s.students.size();
        int processed_students = 0;

        // shuffle students in each subject
        std::shuffle(s.students.begin(), s.students.end(), rand);

        while (student_count > 0) {
            timetable_day_t tutorial_day = this->day_distribution(rand);
            timetable_hour_t tutorial_start_hour = this->contiguous_hour_distribution_tutorials(rand);
            import::Classroom& tut_clrm = tutorial_classrooms[tutorial_classroom_index_distribution(rand)];

            std::shared_ptr<TimetableEntry> te(new TimetableEntry());

            te->day = tutorial_day;
            te->hour = tutorial_start_hour;
            te->subject = s.id;
            te->lectures = false;
            te->classroom = tut_clrm.id;

            std::vector<timetable_student_t>::const_iterator from = s.students.begin() + processed_students;
            std::vector<timetable_student_t>::const_iterator to =
                                            (tut_clrm.tutorial_capacity >= (s.students.size() - processed_students)
                                                  ? s.students.end()
                                                  : s.students.begin() + processed_students + tut_clrm.tutorial_capacity);
            te->students.insert(from, to);

            te->professors.insert(s.teaching_assistants[assistant_index_distribution(rand)]);

            // another (must be double)
            std::shared_ptr<TimetableEntry> matching = te->clone();
            matching->hour += 1;

            timetable->timetable_entries.push_back(te);
            timetable->timetable_entries.push_back(matching);


            processed_students += tut_clrm.tutorial_capacity;
            student_count -= tut_clrm.tutorial_capacity;
        }
    }

    return timetable;
}
