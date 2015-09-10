#include "fitness.h"
#include "../utils.h"

inline void fitness_t::operator+=(double what) {
    this->fitness += what;
}

bool fitness_t::operator<(const fitness_t& other) const {
    return this->fitness < other.fitness;
}

void fitness_t::print_details() {
    std::cout << "Individual fitness: " << this->fitness << std::endl;
    std::cout << "\t" << "non-attached lectures: " << this->non_attached_lecture << std::endl;
    std::cout << "\t" << "too early start: " << this->start_too_early << std::endl;
    std::cout << "\t" << "too late end: " << this->end_too_late << std::endl;
    std::cout << "\t" << "too late end (soft): " << this->end_too_late_soft << std::endl;
    std::cout << "\t" << "classroom over capacity: " << this->classroom_over_capacity << std::endl;
    std::cout << "\t" << "timetable entry overlap: " << this->timetable_entry_overlap << std::endl;
    std::cout << "\t" << "professor overlap: " << this->professor_overlap << std::endl;
    std::cout << "\t" << "student overlap: " << this->student_overlap << std::endl;
    std::cout << "\t" << "subject lecture tutorials overlap: " << this->subject_lecture_tutorials_overlap << std::endl;
    std::cout << "\t" << "subject lectures overlap: " << this->subject_lecture_overlap << std::endl;
    std::cout << "\t" << "tutorials not double cycle: " << this->tutorials_double_cycle << std::endl;
    std::cout << "\t" << "professor over load: " << this->professor_over_load << std::endl;
    std::cout << "\t" << "student preferred start bonuses: " << this->student_preferred_start << std::endl;
    std::cout << "\t" << "student preferred end bonuses: " << this->student_preferred_end << std::endl;
    std::cout << "\t" << "number of merged lectures: " << this->lectures_merged << std::endl;
    std::cout << "\t" << "number of tutorials after lectures: " << this->tutorials_after_lectures << std::endl;
    std::cout << "\t" << "student entry grouping variance smaller: " << this->student_entry_grouping_variance_smaller << std::endl;
    std::cout << "\t" << "student entry grouping variance larger: " << this->student_entry_grouping_variance_larger << std::endl;
}

FitnessCore::FitnessCore(std::map<int, import::Professor>& professors,
                         std::map<int, import::Classroom>& classrooms,
                         std::map<int, import::Student>& students,
                         std::map<int, import::Subject>& subjects) {
    this->professors = professors;
    this->classrooms = classrooms;
    this->students = students;
    this->subjects = subjects;

    this->professor_loads = std::map<timetable_professor_t, unsigned int>();
    this->subject_lecture_ends = std::map<int, std::pair<timetable_day_t, timetable_hour_t>>();
    for (auto i : this->subjects) {
        this->subject_lecture_ends[i.first] = std::make_pair((timetable_day_t) 0, (timetable_hour_t) 0);
    }

    this->tutorial_has_pair = std::set<int>();

    this->student_start_limit_conformities = std::map<timetable_student_t, bool>();
    this->student_end_limit_conformities = std::map<timetable_student_t, bool>();

    this->student_entry_times = std::map<timetable_student_t, std::vector<int>>();
    for (auto s : this->students) {
        this->student_entry_times[s.first] = std::vector<int>();
    }

    this->reset_utilities();
}

void FitnessCore::reset_utilities() {
    for (auto i : this->professors) {
        this->professor_loads[i.first] = 0;
    }
    for (auto i : this->students) {
        this->student_start_limit_conformities[i.first] = true;
        this->student_end_limit_conformities[i.first] = true;
        this->student_entry_times.clear();
    }
    for (auto i : this->subjects) {
        this->subject_lecture_ends[i.first].first = 0;
        this->subject_lecture_ends[i.first].second = 0;
    }
    this->tutorial_has_pair.clear();
}

fitness_t FitnessCore::calculate_fitness(std::shared_ptr<Timetable>& timetable) {
    reset_utilities();
    fitness_t result = fitness_t();

    // calculating fitness requires the timetable to be sorted
    timetable->sort();

    int outer_counter = 0;
    std::shared_ptr<TimetableEntry> saved_entry = timetable->timetable_entries.front();
    for (auto outer = timetable->timetable_entries.begin(); outer != timetable->timetable_entries.end(); outer++, outer_counter++) {
        std::shared_ptr<TimetableEntry>& e1 = *outer;

        // start and end times
        if (e1->hour < EARLIEST_HOUR) {
            result += START_TOO_EARLY_SCORE;
            result.start_too_early++;
        }
        if (e1->hour > LATEST_HOUR) {
            result += END_TOO_LATE_SCORE;
            result.end_too_late++;
        } else if (e1->hour > SOFT_LATEST_HOUR) {
            result += SOFT_LATEST_HOUR_SCORE;
            result.end_too_late_soft++;
        }

        // professor loads (only for tutorials, lectures do not count
        for (int p : e1->professors) {
            if (!e1->lectures) {
                this->professor_loads[p]++;
            }
        }

        // classroom capacity check
        if ((e1->lectures && e1->students.size() > this->classrooms[e1->classroom].lecture_capacity)
                || (!e1->lectures && e1->students.size() > this->classrooms[e1->classroom].tutorial_capacity)) {
            result += CLASSROOM_OVER_CAPACITY_SCORE;
            result.classroom_over_capacity++;
        }

        // lecture and tutorial linear checks
        if (e1->lectures) {
            // if the lectures are of the same subject on the same day, and also are apart more than an hour (non-sequential),
            // apply a penalty (this works because we always compare two time-sequential lectures
            if (e1->subject == saved_entry->subject && e1->day == saved_entry->day && e1->hour - saved_entry->hour > 1) {
                result += NON_ATTACHED_LECTURE_SCORE;
                result.non_attached_lecture++;
            }

            saved_entry = e1;
        }

        // tutorials after lectures bonus
        if (e1->day > this->subject_lecture_ends[e1->subject].first ||
            (e1->day == this->subject_lecture_ends[e1->subject].first && e1->hour > this->subject_lecture_ends[e1->subject].second)) {
            if (e1->lectures) {
                // store the latest lecture time for the subject so we can compute bonuses for tutorials being after lectures
                this->subject_lecture_ends[e1->subject].first = e1->day;
                this->subject_lecture_ends[e1->subject].second = e1->hour;
            } else {
                // compare the time if this tutorial to the latest lecture
                // this works because the vector is sorted so all lectures appear before tutorials within the same subject
                result += TUTORIALS_AFTER_LECTURES_BONUS;
                result.tutorials_after_lectures++;
            }
        }

        // calculate the contiguous slot time representation for variance calculation
        int packed_variance_time = utils::get_packed_slot_time(e1->day, e1->hour, EARLIEST_HOUR, LATEST_HOUR);

        // student operations
        for (int s : e1->students) {
            // check start and end conformities: bonus points for students always starting after or always ending before a specified hour
            if (e1->hour < STUDENT_PREFERRED_START) {
                this->student_start_limit_conformities[s] = false;
            }
            if (e1->hour > STUDENT_PREFERRED_END) {
                this->student_end_limit_conformities[s] = false;
            }

            this->student_entry_times[s].push_back(packed_variance_time);
        }

        // only tutorials have this
        bool found_tutorial_match = e1->lectures;

        // can compare each pair
        auto inner = outer;
        int inner_counter = outer_counter + 1;
        for (inner++; inner != timetable->timetable_entries.end(); inner++, inner_counter++) {
            std::shared_ptr<TimetableEntry>& e2 = *inner;

            // entries occur at the same time
            if (e1->day == e2->day && e1->hour == e2->hour) {
                // you can't have two entries in the same classroom
                if (e1->classroom == e2->classroom) {
                    result += TIMETABLE_ENTRY_OVERLAP_SCORE;
                    result.timetable_entry_overlap++;
                }

                int overlapping_professors = utils::count_overlaps(e1->professors, e2->professors);
                if (overlapping_professors > 0) {
                    result += PROFESSOR_OVERLAP_SCORE;
                    result.professor_overlap++;
                }

                // a student shouldn't be in two places at the same time
                int student_overlaps = utils::count_overlaps(e1->students, e2->students);
                result += student_overlaps * STUDENT_OVERLAP_SCORE;
                result.student_overlap += student_overlaps;

                // the same subject can't have tutorials and lectures at the same time
                if (e1->subject == e2->subject) {
                    if (e1->lectures && !e2->lectures) {
                        result += SUBJECT_LECTURE_TUTORIALS_OVERLAP_SCORE;
                        result.subject_lecture_tutorials_overlap++;
                    }

                    if (e1->lectures && e2->lectures) {
                        result += SUBJECT_LECTURE_OVERLAP_SCORE;
                        result.subject_lecture_overlap++;
                    }
                }
            }

            // see the tutorial_has_pair initialization for reasoning
            // we only check outer_counter because it is the element being checked
            // and we have already (probably) found its pair before
            if (!found_tutorial_match && (e1->is_matching_tutorial(e2) || this->tutorial_has_pair.count(outer_counter) == 1)) {
                found_tutorial_match = true;
                this->tutorial_has_pair.insert(inner_counter);
                this->tutorial_has_pair.insert(outer_counter);
            }

            // bonus points for each matching lecture pair (enforces them to be merged)
            if (e1->is_matching_lecture_strict(e2)) {
                result += LECTURES_MERGED_BONUS;
                result.lectures_merged++;
            }
        }

        // if two tutorials aren't grouped together, apply a penalty
        // lectures are handled at the found_tutorial_match definition
        if (!found_tutorial_match) {
            result += TUTORIALS_NOT_DOUBLE_CYCLE_SCORE;
            result.tutorials_double_cycle++;
        }
    }

    // professor loads post-processing
    for (auto p : this->professor_loads) {
        if (p.second > this->professors[p.first].available_hours) {
            result += PROFESSOR_OVER_LOAD_SCORE;
            result.professor_over_load++;
        }
    }

    // student preferred times post-processing
    for (auto i : this->student_start_limit_conformities) {
        if (i.second) {
            result += STUDENT_PREFERRED_START_BONUS;
            result.student_preferred_start++;
        }
    }
    for (auto i : this->student_end_limit_conformities) {
        if (i.second) {
            result += STUDENT_PREFERRED_END_BONUS;
            result.student_preferred_end++;
        }
    }

    // variance (student entry grouping) post-processing
    for (auto& i : this->student_entry_times) {
        // calculate the variance for each student
        // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Online_algorithm
        int n = 0;
        double mean = 0.0;
        double variance = 0.0;

        for (int time : i.second) {
            n++;
            double delta = time - mean;
            mean += delta / n;
            variance += delta * (time - mean);
        }

        if (n < 2) {
            variance = 0;
        } else {
            variance /= n;
        }

        // now evaluate the variance
        // take the variance of an uniform distribution as the "maximum"
        double uniform_variance = pow(utils::get_packed_slot_time(4, LATEST_HOUR, EARLIEST_HOUR, LATEST_HOUR), 2) / 12;

        double variance_difference_normalized = (uniform_variance - variance) / uniform_variance;

        // we add a positive score (better) if the variance is smaller (better) than the uniform variance
        // we essentially subtract if the variance is larger (that's worse)
        result += variance_difference_normalized * STUDENT_ENTRY_GROUPING_SCORE;
        if (variance_difference_normalized < 0) {
            result.student_entry_grouping_variance_larger++;
        } else {
            result.student_entry_grouping_variance_smaller++;
        }
    }

    // this seems to be a bug, so a workaround is needed
    if (result.tutorials_double_cycle == 1) {
        result.tutorials_double_cycle = 0;
        result.fitness -= TUTORIALS_NOT_DOUBLE_CYCLE_SCORE;
    }

    return result;
}

bool FitnessPair::compare_fitness(const FitnessPair& a, const FitnessPair& b) {
    return a.fitness > b.fitness;
}
