import xml.etree.ElementTree as ET
from xml.dom import minidom
from random import shuffle


# the number of students in each year, implicitly defines the number of years
year_students = [100, 75, 60]

# each year has a number of subjects (only count one semester)
year_subjects = [5, 5, 10]

# the number of subjects in a year that are mandatory
# mandatory subjects have all students of a year assigned, for non-mandatory,
# all students of that year are evenly distributed between them
year_subjects_mandatory = [5, 5, 1]

assert len(year_students) == len(year_subjects) == len(year_subjects_mandatory)

# each subject gets its own professor
# assistants have to be specified
# jobs are randomly distributed: each assistant gets one subject,
# then the remaining subjects are evenly distributed (think balanced tree)
num_assistants = 10
global_assistant_available_hours = 20
assert sum(year_subjects) > num_assistants

# the types of the classrooms
TYPE_LECTURE = 0
TYPE_TUTORIAL = 1
TYPE_LECTURE_TUTORIAL = 2

# each classroom has a capacity and a type
classroom_capacities = [150, 120, 30, 30, 18, 18, 18]
classroom_types = [TYPE_LECTURE, TYPE_LECTURE, TYPE_LECTURE_TUTORIAL, TYPE_LECTURE_TUTORIAL, TYPE_TUTORIAL, TYPE_TUTORIAL, TYPE_TUTORIAL]
assert len(classroom_capacities) == len(classroom_types)


# results
classrooms = []
professors = []
students = []
subjects = []

FILE_SAVE_LOCATION = "./xml/"
CLASSROOMS_XML_NAME = "classrooms.xml"
PROFESSORS_XML_NAME = "professors.xml"
STUDENTS_XML_NAME = "students.xml"
SUBJECTS_XML_NAME = "subjects.xml"


# entities that directly translate into XML
class Classroom:
    def __init__(self):
        self.id = None
        self.capacity = None

class Professor:
    def __init__(self):
        self.id = None
        self.name = None
        self.available_hours = None

class Student:
    def __init__(self):
        self.id = None
        self.subjects = []

class Subject:
    def __init__(self):
        self.id = None
        self.lecture_classrooms = []
        self.tutorial_classrooms = []
        self.professors = []
        self.assistants = []


# computation
assistant_start_index = sum(year_subjects)

for i, cap in enumerate(classroom_capacities):
    c = Classroom()
    c.id = i
    c.capacity = cap
    classrooms.append(c)

global_subject_index = 0
global_teacher_index = 0
for i in range(len(year_students)):
    student_count = year_students[i]
    subject_count = year_subjects[i]
    subjects_mandatory = year_subjects_mandatory[i]

    subject_objects_this_year = []

    # for each year (semester), create all subjects
    for s in range(subject_count):
        sub = Subject()
        sub.id = global_subject_index
        global_subject_index += 1

        for c in range(len(classroom_capacities)):
            classroom_capacity = classroom_capacities[c]
            classroom_type = classroom_types[c]

            if classroom_type == TYPE_LECTURE or classroom_type == TYPE_LECTURE_TUTORIAL:
                sub.lecture_classrooms.append(c)

            if classroom_type == TYPE_TUTORIAL or classroom_type == TYPE_LECTURE_TUTORIAL:
                sub.tutorial_classrooms.append(c)

        # create a professor for this
        sub.professors.append(global_teacher_index)
        global_teacher_index += 1

        subject_objects_this_year.append(sub)

    # assign students to this year's subjects
    student_start_index = sum(year_students[:i])
    for idx_stud, s in enumerate(range(student_count)):
        subj = Student()
        subj.id = s + student_start_index

        for idx_sub, sub in enumerate(subject_objects_this_year):
            # the subject can either be mandatory or optional
            if idx_sub < subjects_mandatory:
                # this is a mandatory subject
                # the student gets the subject unconditionally
                subj.subjects.append(sub.id)
            else:
                # this is an optional subject
                # assigning optional subjects is elsewhere
                pass

        # handle optional subjects: randomly assign
        if subject_count != subjects_mandatory:
            optional_subject_count = subject_count - subjects_mandatory
            optional_subjects = subject_objects_this_year[subjects_mandatory::]

            student_remaining_subjects = 5 - subjects_mandatory
            assert student_remaining_subjects < optional_subject_count

            shuffle(optional_subjects)
            for s in optional_subjects[:student_remaining_subjects]:
                subj.subjects.append(s.id)

        students.append(subj)

    # copy the year subjects into the global subject container
    for s in subject_objects_this_year:
        subjects.append(s)


# after all subjects have been created, assign assistants to them (uniformly)
for s, sub in enumerate(subjects):
    sub.assistants.append((s % num_assistants) + global_teacher_index)  # +global teacher index because professors and TAs use the same index

# create all professors (traverse subjects and create profs and tas) (capacity is uniform)
util_processed_assistants = []
for s in subjects:
    for p in s.professors:
        prof = Professor()
        prof.id = p
        prof.name = "prof " + str(prof.id)
        prof.available_hours = 0  # professors don't need this limitation

        professors.append(prof)

    for a in s.assistants:
        # don't enter duplicate assistants (professors are unique to the subjects)
        if a in util_processed_assistants:
            continue
        util_processed_assistants.append(a)

        ass = Professor()
        ass.id = a
        ass.name = "assistant " + str(ass.id - global_teacher_index)
        ass.available_hours = global_assistant_available_hours

        professors.append(ass)


def save_pretty(xml_root, directory, name):
    xml_root.set("xmlns", "http://stanovnik.net/ParallelTimetables")
    xml_root.set("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance")
    xml_root.set("xsi:schemaLocation", "http://stanovnik.net/ParallelTimetables " + xml_root.tag + ".xsd")

    f = open(directory + "/" + name, "w")
    xml_string = ET.tostring(xml_root, encoding="UTF-8")
    f.write(minidom.parseString(xml_string).toprettyxml(indent="    "))
    f.close()


# write out the XML files
clrm_root = ET.Element("classrooms")
for clrm in classrooms:
    clrm_element = ET.SubElement(clrm_root, "classroom")
    clrm_element.set("id", str(clrm.id))

    capacity_element = ET.SubElement(clrm_element, "capacity")
    capacity_element.text = str(clrm.capacity)
save_pretty(clrm_root, FILE_SAVE_LOCATION, CLASSROOMS_XML_NAME)

prof_root = ET.Element("professors")
for prof in professors:
    prof_element = ET.SubElement(prof_root, "professor")
    prof_element.set("id", str(prof.id))

    name_element = ET.SubElement(prof_element, "name")
    name_element.text = str(prof.name)

    hours_element = ET.SubElement(prof_element, "available_hours")
    hours_element.text = str(prof.available_hours)
save_pretty(prof_root, FILE_SAVE_LOCATION, PROFESSORS_XML_NAME)

stud_root = ET.Element("students")
for subj in students:
    subj_element = ET.SubElement(stud_root, "student")
    subj_element.set("id", str(subj.id))

    lecture_classrooms_container = ET.SubElement(subj_element, "subjects")
    for s in subj.subjects:
        id_element = ET.SubElement(lecture_classrooms_container, "id")
        id_element.text = str(s)
save_pretty(stud_root, FILE_SAVE_LOCATION, STUDENTS_XML_NAME)

subj_root = ET.Element("subjects")
for subj in subjects:
    subj_element = ET.SubElement(subj_root, "subject")
    subj_element.set("id", str(subj.id))

    lecture_classrooms_container = ET.SubElement(subj_element, "lecture_classrooms")
    for s in subj.lecture_classrooms:
        id_element = ET.SubElement(lecture_classrooms_container, "id")
        id_element.text = str(s)

    tutorial_classrooms_container = ET.SubElement(subj_element, "tutorial_classrooms")
    for s in subj.tutorial_classrooms:
        id_element = ET.SubElement(tutorial_classrooms_container, "id")
        id_element.text = str(s)

    professors_container = ET.SubElement(subj_element, "professors")
    for s in subj.professors:
        id_element = ET.SubElement(professors_container, "id")
        id_element.text = str(s)

    assistants_container = ET.SubElement(subj_element, "assistants")
    for s in subj.assistants:
        id_element = ET.SubElement(assistants_container, "id")
        id_element.text = str(s)
save_pretty(subj_root, FILE_SAVE_LOCATION, SUBJECTS_XML_NAME)

