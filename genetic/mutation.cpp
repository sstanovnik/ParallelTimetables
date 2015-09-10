#include "mutation.h"
#include "../utils.h"

inline timetable_classroom_t MutationCore::get_random_lecture_classroom(timetable_subject_t subject_id) {
    return this->subject_lecture_classrooms[subject_id][this->subject_lecture_classroom_distributions[subject_id](this->rand)];
}

inline timetable_classroom_t MutationCore::get_random_tutorial_classroom(timetable_subject_t subject_id) {
    return this->subject_tutorial_classrooms[subject_id][this->subject_tutorial_classroom_distributions[subject_id](this->rand)];
}

MutationCore::MutationCore(timetable_hour_t min_hour, timetable_hour_t max_hour, timetable_day_t min_day, timetable_day_t max_day, std::vector<import::Subject>& imported_subjects) {
    this->min_hour = min_hour;
    this->max_hour = max_hour;
    this->min_day = min_day;
    this->max_day = max_day;
    this->imported_subjects = std::shared_ptr<std::vector<import::Subject>>(&imported_subjects);
    this->subject_lecture_classrooms = std::map<timetable_subject_t, std::vector<timetable_classroom_t>>();
    this->subject_tutorial_classrooms = std::map<timetable_subject_t, std::vector<timetable_classroom_t >>();

    this->rand = std::mt19937(utils::get_random_seed());
    this->mutation_point_distribution = std::uniform_int_distribution<int>(0, 5);
    this->day_distribution = std::uniform_int_distribution<timetable_day_t>((timetable_day_t) this->min_day, (timetable_day_t) this->max_day);
    this->hour_distribution = std::uniform_int_distribution<timetable_hour_t>((timetable_hour_t) this->min_hour, (timetable_hour_t) this->max_hour);
    this->zero_one_distribution = std::uniform_real_distribution<double>(0, 1);

    this->subject_lecture_classroom_distributions = std::map<timetable_subject_t , std::uniform_int_distribution<timetable_classroom_t>>();
    this->subject_tutorial_classroom_distributions = std::map<timetable_subject_t , std::uniform_int_distribution<timetable_classroom_t>>();
    for (auto i : *this->imported_subjects) {
        this->subject_lecture_classrooms[i.id] = std::vector<timetable_classroom_t>(i.lecture_classrooms);
        this->subject_tutorial_classrooms[i.id] = std::vector<timetable_classroom_t>(i.tutorial_classrooms);

        this->subject_lecture_classroom_distributions[i.id] = std::uniform_int_distribution<timetable_classroom_t >(0, (timetable_classroom_t) (this->subject_lecture_classrooms[i.id].size() - 1));
        this->subject_tutorial_classroom_distributions[i.id] = std::uniform_int_distribution<timetable_classroom_t>(0, (timetable_classroom_t) (this->subject_tutorial_classrooms[i.id].size() - 1));
    }
}

std::shared_ptr<Timetable> MutationCore::perform_mutation(std::shared_ptr<Timetable>& parent) {
    // the initial data is a clone, then we modify it
    std::shared_ptr<Timetable> result = parent->clone();

    // this is supposed to be lightweight enough
    int entry_index = std::uniform_int_distribution<int>(0, (int) (result->timetable_entries.size() - 1))(rand);

    int mutation_type = mutation_point_distribution(rand);
    std::shared_ptr<TimetableEntry>& entry = result->timetable_entries[entry_index];
    switch (mutation_type) {
        case 0: { // classroom change (lecture or tutorial, depending on the type)
            // regardless of the type, all entries that match this one and are attached to it are changed
            // as this makes the most sense domain-wise
            if (entry->lectures) {
                timetable_classroom_t new_classroom = get_random_lecture_classroom(entry->subject);
                for (std::shared_ptr<TimetableEntry>& e : result->timetable_entries) {
                    // for lectures, we change all lecture entries for the same subject within 2 hours of eachother
                    if (e->is_matching_lecture(entry)) {
                        e->classroom = new_classroom;
                    }
                }
                entry->classroom = new_classroom;
            } else {
                timetable_classroom_t new_classroom = get_random_tutorial_classroom(entry->subject);
                bool tutorial_found = false;
                for (std::shared_ptr<TimetableEntry>& e : result->timetable_entries) {
                    // for lectures, we change all lecture entries for the same subject within 2 hours of eachother
                    if (e->is_matching_tutorial(entry)) {
                        e->classroom = new_classroom;
                        tutorial_found = true;
                        break;
                    }
                }
                entry->classroom = new_classroom;

                if (!tutorial_found) {
                    std::cerr << "Matching tutorial not found (classroom mutation). " << std::endl;
                    return nullptr;
                }
            }
            break;
        }
        case 1: { // day change
            // if the entry is a tutorial, also change the matching tutorial entry's time
            // don't bother checking if the other entry's time is valid (not too soon, not too late)
            // as this is checked by the fitness function
            timetable_day_t new_day = day_distribution(rand);


            // only mutate matching if it's a tutorial
            if (!entry->lectures) {
                bool tutorial_found = false;
                for (std::shared_ptr<TimetableEntry>& e : result->timetable_entries) {
                    if (e->is_matching_tutorial(entry)) {
                        e->day = new_day;
                        tutorial_found = true;
                        break;
                    }
                }

                if (!tutorial_found) {
                    std::cerr << "Matching tutorial not found (day mutation). " << std::endl;
                    return nullptr;
                }
            }

            entry->day = new_day;

            break;
        }
        case 2: { // hour change
            // see day comment
            timetable_hour_t new_hour = hour_distribution(rand);

            // only mutate matching if it's a tutorial
            if (!entry->lectures) {
                bool tutorial_found = false;
                for (std::shared_ptr<TimetableEntry>& e : result->timetable_entries) {
                    if (e->is_matching_tutorial(entry)) {
                        if (e->hour < entry->hour) {
                            e->hour = (timetable_hour_t) (new_hour - 1);
                        } else {
                            e->hour = (timetable_hour_t) (new_hour + 1);
                        }
                        tutorial_found = true;
                        break;
                    }
                }

                if (!tutorial_found) {
                    std::cerr << "Matching tutorial not found (hour mutation). " << std::endl;
                    return nullptr;
                }
            }

            entry->hour = new_hour;
            break;
        }
        case 3: { // day and hour change
            // see day comment
            timetable_day_t new_day = day_distribution(rand);
            timetable_hour_t new_hour = hour_distribution(rand);

            // only mutate matching if it's a tutorial
            if (!entry->lectures) {
                bool tutorial_found = false;
                for (std::shared_ptr<TimetableEntry>& e : result->timetable_entries) {
                    if (e->is_matching_tutorial(entry)) {
                        e->day = new_day;
                        if (e->hour < entry->hour) {
                            e->hour = (timetable_hour_t) (new_hour - 1);
                        } else {
                            e->hour = (timetable_hour_t) (new_hour + 1);
                        }
                        tutorial_found = true;
                        break;
                    }
                }

                if (!tutorial_found) {
                    std::cerr << "Matching tutorial not found (day and hour mutation). " << std::endl;
                    return nullptr;
                }
            }

            entry->day = new_day;
            entry->hour = new_hour;
            break;
        }
        case 4: { // shuffle students of two same-subject entries
            // don't do anything on lectures
            if (entry->lectures) {
                break;
            }

            // check if we can even swap (if there is more than one pair of tutorials)
            std::vector<int> tutorial_indices = std::vector<int>();
            int te_index = 0;
            for (auto& te : result->timetable_entries) {
                if (!te->lectures && te->subject == entry->subject) {
                    tutorial_indices.push_back(te_index);
                }
                te_index++;
            }

            // we need at least 4 tutorials (two pairs)
            if (tutorial_indices.size() < 4) {
                break;
            }

            // get the matching pair for the original entry
            std::shared_ptr<TimetableEntry> entry_matching;
            bool entry_matching_found = false;
            for (std::shared_ptr<TimetableEntry>& e : result->timetable_entries) {
                if (e->is_matching_tutorial(entry)) {
                    entry_matching = e;
                    entry_matching_found = true;
                    break;
                }
            }
            if (!entry_matching_found) {
                std::cerr << "Matching entry not found (student mutation). " << std::endl;
                return nullptr;
            }

            // remove the entry and its matching entry
            // this way only valid matches are left
            auto new_end = std::remove_if(tutorial_indices.begin(),
                                          tutorial_indices.end(),
                                          [&result, &entry](int idx) {
                                              std::shared_ptr<TimetableEntry>& te = result->timetable_entries[idx];
                                              return te->classroom == entry->classroom
                                                  && te->students == entry->students;
                                          }
            );
            tutorial_indices.erase(new_end, tutorial_indices.end());


            int other_index = tutorial_indices[std::uniform_int_distribution<int>(0, (int) (tutorial_indices.size() - 1))(rand)];
            std::shared_ptr<TimetableEntry>& other = result->timetable_entries[other_index];
            std::shared_ptr<TimetableEntry> other_matching;
            bool other_matching_found = false;
            for (std::shared_ptr<TimetableEntry>& e : result->timetable_entries) {
                if (e->is_matching_tutorial(other)) {
                    other_matching = e;
                    other_matching_found = true;
                    break;
                }
            }
            if (!other_matching_found) {
                std::cerr << "Matching other entry not found (student mutation). " << std::endl;
                return nullptr;
            }

            // shuffle the students
            int entry_student_count = (int) entry->students.size();
            std::vector<int> merged_students = std::vector<int>();
            merged_students.insert(merged_students.end(), entry->students.begin(), entry->students.end());
            merged_students.insert(merged_students.end(), other->students.begin(), other->students.end());
            std::shuffle(merged_students.begin(), merged_students.end(), rand);

            entry->students.clear();
            entry_matching->students.clear();
            other->students.clear();
            other_matching->students.clear();

            // redistribute with the same numbers as before
            entry->students.insert(merged_students.begin(), merged_students.begin() + entry_student_count);
            entry_matching->students.insert(merged_students.begin(), merged_students.begin() + entry_student_count);
            other->students.insert(merged_students.begin() + entry_student_count, merged_students.end());
            other_matching->students.insert(merged_students.begin() + entry_student_count, merged_students.end());

            break;
        }
        case 5: { // TA swap
            // don't do anything for lectures
            if (entry->lectures) {
                break;
            }

            // choose another random TA here
            import::Subject subject;
            bool subject_found = false;
            for (import::Subject i : *this->imported_subjects) {
                if (i.id == entry->subject) {
                    subject_found = true;
                    subject = i;
                    break;
                }
            }
            if (!subject_found) {
                std::cerr << "Subject not found (TA mutation). " << std::endl;
                throw std::exception();
            }

            // we can't swap if we just swap with the same person
            if (subject.teaching_assistants.size() == entry->professors.size()) {
                break;
            }

            double random_value = zero_one_distribution(rand);
            size_t i;
            double weight_sum = 0;
            for (i = 0; i < subject.teaching_assistants.size(); i++) {
                weight_sum += subject.teaching_assistant_weights[i];
                if (random_value <= weight_sum) {
                    break;
                }
            }
            timetable_professor_t new_ta = subject.teaching_assistants[i];

            // choose which TA to swap
            // prioritize swapping with oneself (that means no swap occurs)
            if (entry->professors.count(new_ta) == 1) {
                break;
            } else {
                // get the matching entry
                bool matching_found = false;
                std::shared_ptr<TimetableEntry> match;
                for (std::shared_ptr<TimetableEntry>& e : result->timetable_entries) {
                    if (entry->is_matching_tutorial(e)) {
                        matching_found = true;
                        match = e;
                        break;
                    }
                }
                if (!matching_found) {
                    std::cerr << "No matching entry found (TA mutation). " << std::endl;
                    return nullptr;
                }

                // choose which TA to swap
                int swap_index = std::uniform_int_distribution<int>(0, (int) (entry->professors.size() - 1))(rand);

                // increment the iterators, then delete the elements and add new ones
                std::set<timetable_professor_t>::iterator entry_it = entry->professors.begin();
                for (int s = 0; s < swap_index; s++) {
                    entry_it++;
                }
                timetable_professor_t element = *entry_it;

                entry->professors.erase(element);
                match->professors.erase(element);
                entry->professors.insert(new_ta);
                match->professors.insert(new_ta);
            }
            break;
        }
        default:
            std::cerr << "invalid mutation type (" << mutation_type << ")" << std::endl;
            throw std::exception();
    }

    result->sorted = false;
    return result;
}
