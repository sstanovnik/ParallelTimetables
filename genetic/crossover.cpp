#include "crossover.h"
#include "../utils.h"

CrossoverCore::CrossoverCore(std::vector<import::Subject>& imported_subjects) {
    this->imported_subjects = std::shared_ptr<std::vector<import::Subject>>(&imported_subjects);

    this->rand = std::mt19937(utils::get_random_seed());
    this->mutation_point_distribution = std::uniform_int_distribution<int>(0, 3);
    this->zero_one_distribution = std::uniform_real_distribution<double>(0, 1);
}

std::shared_ptr<Timetable> CrossoverCore::perform_crossover(std::shared_ptr<Timetable>& left,
                                                            std::shared_ptr<Timetable>& right) {
    std::shared_ptr<Timetable> result(new Timetable());

    int crossover_type = mutation_point_distribution(rand);

    // shared preprocessing
    switch (crossover_type) {
        case 0:
        case 1:
        case 2:
        case 3:
            // initialize the sorted values
            left->sort();
            right->sort();
            break;
        default:
            std::cerr << "invalid mutation place (" << crossover_type << ")" << std::endl;
            throw std::exception();
    }

    // synchronized subject traversal
    std::vector<std::shared_ptr<TimetableEntry>>::iterator left_global_it = left->timetable_entries.begin();
    std::vector<std::shared_ptr<TimetableEntry>>::iterator right_global_it = right->timetable_entries.begin();
    std::vector<std::shared_ptr<TimetableEntry>>::iterator left_global_end = left->timetable_entries.end();
    std::vector<std::shared_ptr<TimetableEntry>>::iterator right_global_end = right->timetable_entries.end();

    for (unsigned int i = 0; i < this->imported_subjects->size(); i++) {
        // pick a random entry between the two
        bool pick_left = this->zero_one_distribution(rand) < 0.5;

        // there are two ways of doing this, but I think this is faster
        // because we don't evaluate an if condition every iteration
        // branch prediction is a thing, but still
        int current_subject = (*left_global_it)->subject;

        if (current_subject != (*this->imported_subjects)[i].id) {
            std::cerr << "subject id mismatch (current " << current_subject << " vs imported " << (*this->imported_subjects)[i].id << " vs right " << (*right_global_it)->subject << ")" << std::endl;
        }

        switch (crossover_type) {
            case 0: { // combine subjects as a whole
                if (pick_left) {
                    while (left_global_it != left_global_end && (*left_global_it)->subject == current_subject) {
                        result->timetable_entries.push_back((*left_global_it)->clone());
                        left_global_it++;
                    }
                    while (right_global_it != right_global_end && (*right_global_it)->subject == current_subject) {
                        right_global_it++;
                    }
                } else {
                    while (left_global_it != left_global_end && (*left_global_it)->subject == current_subject) {
                        left_global_it++;
                    }
                    while (right_global_it != right_global_end && (*right_global_it)->subject == current_subject) {
                        result->timetable_entries.push_back((*right_global_it)->clone());
                        right_global_it++;
                    }
                }
                break;
            }
            case 1:   // students from one subject, everything else from the other
            case 2:   // TAs from one, everything else from the other
            case 3: { // classrooms from one, everything else from the other
                // these are shared as all code is the same except for a few assignments
                std::vector<std::shared_ptr<TimetableEntry>>::iterator left_segment_it = left_global_it;
                std::vector<std::shared_ptr<TimetableEntry>>::iterator right_segment_it = right_global_it;
                int left_segment_count = 0;
                int right_segment_count = 0;

                // get the start-end range for the subject
                while (left_global_it != left_global_end && (*left_global_it)->subject == current_subject) {
                    left_global_it++;
                    left_segment_count++;
                }
                while (right_global_it != right_global_end && (*right_global_it)->subject == current_subject) {
                    right_global_it++;
                    right_segment_count++;
                }

                std::vector<std::shared_ptr<TimetableEntry>>::iterator left_segment_end = left_global_it;
                std::vector<std::shared_ptr<TimetableEntry>>::iterator right_segment_end = right_global_it;

                if (left_segment_count == right_segment_count) {
                    if (pick_left) {
                        for (int j = 0; j < left_segment_count; j++, left_segment_it++, right_segment_it++) {
                            std::shared_ptr<TimetableEntry> clone = (*left_segment_it)->clone();

                            // this is what changes depending on the crossover type
                            if (crossover_type == 1) {
                                clone->students = std::set<timetable_student_t>((*right_segment_it)->students);
                            } else if (crossover_type == 2) {
                                clone->professors = std::set<timetable_professor_t>((*right_segment_it)->professors);
                            } else if (crossover_type == 3) {
                                clone->classroom = (*right_segment_it)->classroom;
                            }

                            result->timetable_entries.push_back(clone);
                        }
                    } else {
                        for (int j = 0; j < right_segment_count; j++, left_segment_it++, right_segment_it++) {
                            std::shared_ptr<TimetableEntry> clone = (*right_segment_it)->clone();

                            // this is what changes depending on the crossover type
                            if (crossover_type == 1) {
                                clone->students = std::set<timetable_student_t>((*left_segment_it)->students);
                            } else if (crossover_type == 2) {
                                clone->professors = std::set<timetable_professor_t>((*left_segment_it)->professors);
                            } else if (crossover_type == 3) {
                                clone->classroom = (*left_segment_it)->classroom;
                            }

                            result->timetable_entries.push_back(clone);
                        }
                    }
                } else {
                    if (pick_left) {
                        while (left_segment_it != left_segment_end) {
                            result->timetable_entries.push_back((*left_segment_it)->clone());
                            left_segment_it++;
                        }
                    } else {
                        while (right_segment_it != right_segment_end) {
                            result->timetable_entries.push_back((*right_segment_it)->clone());
                            right_segment_it++;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    result->sorted = false;
    return result;
}
