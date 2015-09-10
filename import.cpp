#include "import.h"
#include "tinyxml2/tinyxml2.h"

#include <fstream>

std::map<int, import::Professor> import::Professor::import_professors(std::string& file_path) {
    std::map<int, import::Professor> result = std::map<int, import::Professor>();

    tinyxml2::XMLDocument doc;
    doc.LoadFile(file_path.c_str());

    auto *root = doc.FirstChildElement();

    for (auto *professor = root->FirstChildElement(); professor; professor = professor->NextSiblingElement()) {
        import::Professor prof = import::Professor();
        timetable_professor_t id = (timetable_professor_t) professor->IntAttribute("id");
        prof.id = id;

        auto *name = professor->FirstChildElement();
        auto *available_hours = name->NextSiblingElement();

        prof.name = name->GetText();
        prof.available_hours = (unsigned int) atoi(available_hours->GetText());

        result[prof.id] = prof;
    }

    return result;
}

std::map<int, import::Classroom> import::Classroom::import_classrooms(std::string& file_path) {
    std::map<int, import::Classroom> result = std::map<int, import::Classroom>();

    tinyxml2::XMLDocument doc;
    doc.LoadFile(file_path.c_str());

    auto *root = doc.FirstChildElement();

    for (auto *classroom = root->FirstChildElement(); classroom; classroom = classroom->NextSiblingElement()) {
        import::Classroom clrm = import::Classroom();
        timetable_classroom_t id = (timetable_classroom_t) classroom->IntAttribute("id");
        clrm.id = id;

        auto *lecture_capacity = classroom->FirstChildElement();
        auto *tutorial_capacity = lecture_capacity->NextSiblingElement();

        clrm.lecture_capacity = (unsigned int) atoi(lecture_capacity->GetText());
        clrm.tutorial_capacity = (unsigned int) atoi(tutorial_capacity->GetText());

        result[clrm.id] = clrm;
    }

    return result;
}

std::map<int, import::Subject> import::Subject::import_subjects(std::string& file_path) {
    std::map<int, import::Subject> result = std::map<int, import::Subject>();

    tinyxml2::XMLDocument doc;
    doc.LoadFile(file_path.c_str());

    auto *root = doc.FirstChildElement();

    for (auto *subject = root->FirstChildElement(); subject; subject = subject->NextSiblingElement()) {
        import::Subject subj = import::Subject();
        timetable_subject_t id = (timetable_subject_t) subject->IntAttribute("id");
        subj.id = id;

        auto *lecture_classrooms_container = subject->FirstChildElement();
        for (auto *id_container = lecture_classrooms_container->FirstChildElement(); id_container; id_container = id_container->NextSiblingElement()) {
            timetable_classroom_t classroom_id = (timetable_classroom_t) atoi(id_container->GetText());
            subj.lecture_classrooms.push_back(classroom_id);
        }

        auto *tutorial_classrooms_container = lecture_classrooms_container->NextSiblingElement();
        for (auto *id_container = tutorial_classrooms_container->FirstChildElement(); id_container; id_container = id_container->NextSiblingElement()) {
            timetable_classroom_t classroom_id = (timetable_classroom_t) atoi(id_container->GetText());
            subj.tutorial_classrooms.push_back(classroom_id);
        }

        auto *professors_container = tutorial_classrooms_container->NextSiblingElement();
        for (auto *id_container = professors_container->FirstChildElement(); id_container; id_container = id_container->NextSiblingElement()) {
            timetable_professor_t professor_id = (timetable_professor_t) atoi(id_container->GetText());
            subj.professors.push_back(professor_id);
        }

        double weight_sum = 0;
        auto *assistants_container = professors_container->NextSiblingElement();
        for (auto *id_container = assistants_container->FirstChildElement(); id_container; id_container = id_container->NextSiblingElement()) {
            timetable_professor_t professor_id = (timetable_professor_t) atoi(id_container->GetText());
            double weight = id_container->DoubleAttribute("weight"); // returns 0 if not found, that's okay (see below)
            subj.teaching_assistants.push_back(professor_id);
            subj.teaching_assistant_weights.push_back(weight);

            weight_sum += weight;
        }

        if (fabs(weight_sum - 1) > 0.001) {
            std::cerr << "Weights at subject " << ((int) subj.id) << " do not sum to 1. Using a uniform distribution. " << std::endl;

            subj.teaching_assistant_weights.clear();
            for (size_t i = 0; i < subj.teaching_assistants.size(); i++) {
                subj.teaching_assistant_weights.push_back(1.0 / subj.teaching_assistants.size());
            }
        }

        result[subj.id] = subj;
    }

    return result;
}

std::vector<import::Classroom> import::Subject::get_possible_classrooms(std::vector<import::Classroom>& classrooms, bool lectures) {
    std::vector<import::Classroom> res = std::vector<import::Classroom>();
    for (auto c : classrooms) {
        if (lectures && std::find(this->lecture_classrooms.begin(), this->lecture_classrooms.end(), c.id) != this->lecture_classrooms.end()) {
            res.push_back(c);
        } else if (!lectures && std::find(this->tutorial_classrooms.begin(), this->tutorial_classrooms.end(), c.id) != this->tutorial_classrooms.end()) {
            res.push_back(c);
        }
    }
    return res;
}

std::map<int, import::Student> import::Student::import_students(std::string& file_path) {
    std::map<int, import::Student> result = std::map<int, import::Student>();

    tinyxml2::XMLDocument doc;
    doc.LoadFile(file_path.c_str());

    auto *root = doc.FirstChildElement();

    for (auto *student = root->FirstChildElement(); student; student = student->NextSiblingElement()) {
        import::Student stud = import::Student();
        timetable_student_t id = (timetable_student_t) student->IntAttribute("id");
        stud.id = id;

        auto *subjects_container = student->FirstChildElement();
        for (auto *id_container = subjects_container->FirstChildElement(); id_container; id_container = id_container->NextSiblingElement()) {
            timetable_subject_t subject_id = (timetable_subject_t) atoi(id_container->GetText());
            stud.subjects.push_back(subject_id);
        }

        result[stud.id] = stud;
    }

    return result;
}

void import::Subject::populate_students(std::map<int, import::Student> student_map) {
    for (auto i = student_map.begin(); i != student_map.end(); i++) {
        import::Student student = i->second;

        // check if the student has this subject: if so, add this student to the subject
        if (std::find(student.subjects.begin(), student.subjects.end(), this->id) != student.subjects.end()) {
            this->students.push_back(student.id);
        }
    }
}
