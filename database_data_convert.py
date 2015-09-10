
def save_pretty(xml_root, directory, name):
    xml_root.set("xmlns", "http://stanovnik.net/ParallelTimetables")
    xml_root.set("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance")
    xml_root.set("xsi:schemaLocation", "http://stanovnik.net/ParallelTimetables " + xml_root.tag + ".xsd")

    f = open(directory + "/" + name, "w", encoding="UTF-8")
    xml_string = ET.tostring(xml_root, encoding="UTF-8")
    f.write(minidom.parseString(xml_string).toprettyxml(indent="    "))
    f.close()

# entities that directly translate into XML
class Classroom:
    def __init__(self):
        self.original_id = None
        self.generated_id = None
        self.descriptive = None
        self.lecture_capacity = None
        self.tutorial_capacity = None

    def __str__(self, *args, **kwargs):
        return "Classroom {} (org {}): cap {}/{}".format(self.generated_id, self.original_id, self.lecture_capacity, self.tutorial_capacity)


class Professor:
    def __init__(self):
        self.original_id = None
        self.generated_id = None
        self.descriptive = None
        self.name = None
        self.available_hours = None

    def __str__(self, *args, **kwargs):
        return "Professor {} (org {}): name {}, available hours {}".format(self.generated_id, self.original_id, self.name, self.available_hours)


class Student:
    def __init__(self):
        self.original_id = None
        self.generated_id = None
        self.descriptive = None

        # this contains original IDs
        self.subjects = []

    def __str__(self, *args, **kwargs):
        return "Student {} (org {}): subjects ".format(self.generated_id, self.original_id, self.subjects)


class Subject:
    def __init__(self):
        self.original_id = None
        self.generated_id = None
        self.descriptive = None

        # these contain original IDs
        self.lecture_classrooms = []
        self.tutorial_classrooms = []
        self.professors = []
        self.assistants = []

    def __str__(self, *args, **kwargs):
        return "Subject {} (org {}): lecture classrooms {}, tutorial classrooms {}, professors {}, assistants {}".format(self.generated_id, self.original_id, self.lecture_classrooms, self.tutorial_classrooms, self.professors, self.assistants)


class ConvertedResult:
    SUBJECT = 0
    STUDENT = 1
    PROFESSOR = 2
    CLASSROOM = 3

    def __init__(self):
        self.subjects = []
        self.students = []
        self.professors = []
        self.classrooms = []

    def get_by_original_id(self, what, original_id):
        place = None
        if what == ConvertedResult.SUBJECT:
            place = self.subjects
        elif what == ConvertedResult.STUDENT:
            place = self.students
        elif what == ConvertedResult.PROFESSOR:
            place = self.professors
        elif what == ConvertedResult.CLASSROOM:
            place = self.classrooms
        else:
            raise Exception("Invalid type. ")

        for v in place:
            if v.original_id == original_id:
                return v
        raise Exception("Entity not found ({}). ".format(original_id))

    def get_by_generated_id(self, what, generated_id):
        place = None
        if what == ConvertedResult.SUBJECT:
            place = self.subjects
        elif what == ConvertedResult.STUDENT:
            place = self.students
        elif what == ConvertedResult.PROFESSOR:
            place = self.professors
        elif what == ConvertedResult.CLASSROOM:
            place = self.classrooms
        else:
            raise Exception("Invalid type. ")

        for v in place:
            if v.generated_id == generated_id:
                return v
        raise Exception("Entity not found ({}). ".format(generated_id))

    def filter(self):
        """
        Filters out values we can't handle. Hack.
        """
        for stud in self.students:
            for subid in list(stud.subjects):
                sub = self.get_by_original_id(ConvertedResult.SUBJECT, subid)
                if "(EF)" in sub.descriptive or (len(sub.professors) == 0 and len(sub.assistants) == 0):
                    stud.subjects.remove(subid)

        for sub in list(self.subjects):
            if "(EF)" in sub.descriptive or (len(sub.professors) == 0 and len(sub.assistants) == 0):
                self.subjects.remove(sub)

    def save(self, location):
        path = location + "data.pickle"
        with open(path, "wb") as f:
            pickle.dump(self, f, pickle.HIGHEST_PROTOCOL)

    def check_uniqueness(self):
        """
        Check whether all entities have unique IDs to avoid problems.
        """
        assert len(self.subjects) == len(set([s.original_id for s in self.subjects])) and len(self.subjects) != 0
        assert len(self.students) == len(set([s.original_id for s in self.students])) and len(self.students) != 0
        assert len(self.professors) == len(set([s.original_id for s in self.professors])) and len(self.professors) != 0
        assert len(self.classrooms) == len(set([s.original_id for s in self.classrooms])) and len(self.classrooms) != 0

    def get_students(self, cur):
        cur.execute("SELECT DISTINCT stud.\"studentId\", stud.name, stud.surname "
                    "FROM friprosveta_student AS stud, friprosveta_studentenrollment AS enr, timetable_groupset AS grp "
                    "WHERE     stud.id = enr.student_id"
                    "      AND enr.groupset_id = grp.id"
                    "      AND grp.name = 'FRI 2014/2015, letni semester'")
        res_stud = cur.fetchall()

        seq = 0
        for tup in res_stud:
            sid = tup["studentId"]

            stud = Student()
            stud.original_id = sid
            stud.generated_id = seq
            stud.descriptive = "{} {} ({})".format(tup["name"], tup["surname"], sid)

            cur.execute("SELECT DISTINCT subj.* "
                        "FROM friprosveta_student AS stud, friprosveta_studentenrollment AS enr, timetable_groupset AS grp, friprosveta_subject AS subj "
                        "WHERE     stud.id = enr.student_id "
                        "      AND enr.groupset_id = grp.id "
                        "      AND enr.subject_id = subj.id "
                        "      AND grp.name = 'FRI 2014/2015, letni semester' "
                        "      AND stud.\"studentId\" = %s;", (sid,))
            subs = [s["code"] for s in cur.fetchall()]
            stud.subjects = subs

            seq += 1
            self.students.append(stud)
        assert seq <= 256 * 256 - 1

    def get_classrooms(self, cur):
        cur.execute("SELECT c.id, c.capacity, c.\"shortName\" "
                    "FROM timetable_classroom c, timetable_classroomset cs, timetable_classroomset_classrooms csc "
                    "WHERE     c.id = csc.classroom_id "
                    "      AND cs.id = csc.classroomset_id "
                    "      AND cs.name = 'FRI rooms & velika fizikalna predavalnica'")
        res = cur.fetchall()

        seq = 0
        for c in res:
            clrm = Classroom()
            clrm.original_id = c["id"]
            clrm.generated_id = seq

            clrm.lecture_capacity = int(c["capacity"])
            clrm.descriptive = c["shortName"]

            # process tutorial capacity
            if clrm.lecture_capacity > 50:
                clrm.tutorial_capacity = clrm.lecture_capacity  # use this because it's later skipped
            elif clrm.lecture_capacity >= 30:
                clrm.tutorial_capacity = 30
            elif clrm.lecture_capacity >= 18:
                clrm.tutorial_capacity = 18
            else:
                clrm.tutorial_capacity = clrm.lecture_capacity

            seq += 1
            self.classrooms.append(clrm)
        assert seq <= 255

    def get_professors(self, cur):
        cur.execute("SELECT DISTINCT tea.code, auth_user.first_name, auth_user.last_name "
                    "FROM friprosveta_subject sub, friprosveta_activity act1, friprosveta_lecturetype lectype, timetable_activity act2, "
                    "     timetable_activityset actset, timetable_activityrealization actrea, timetable_activityrealization_teachers actreatea, "
                    "     timetable_teacher tea, auth_user "
                    "WHERE     sub.id = act1.subject_id "
                    "AND act1.lecture_type_id = lectype.id "
                    "AND act1.activity_ptr_id = act2.id "
                    "AND act2.activityset_id = actset.id "
                    "AND actset.name = 'FRI 2014/2015, letni semester' "  # semester filter
                    "AND act2.id = actrea.activity_id "
                    "AND actrea.id = actreatea.activityrealization_id "
                    "AND actreatea.teacher_id = tea.id "
                    "AND tea.user_id = auth_user.id;")
        res = cur.fetchall()

        seq = 0
        for p in res:
            prof = Professor()
            prof.original_id = p["code"]
            prof.generated_id = seq

            prof.name = "{} {}".format(p["first_name"], p["last_name"])
            prof.descriptive = prof.name

            # limits aren't defined and the assistants can rearrange themselves
            prof.available_hours = 100

            seq += 1
            self.professors.append(prof)
        assert seq <= 255

    def get_subjects(self, cur):
        # get all subjects
        cur.execute("SELECT DISTINCT subj.* "
                    "FROM friprosveta_studentenrollment AS enr, timetable_groupset AS grp, friprosveta_subject AS subj "
                    "WHERE     enr.groupset_id = grp.id "
                    "      AND subj.id = enr.subject_id "
                    "      AND grp.name = 'FRI 2014/2015, letni semester'")
        res_subjects = cur.fetchall()

        seq = 0
        for s in res_subjects:
            subj = Subject()
            subj.original_id = s["code"]
            subj.generated_id = seq

            subj.descriptive = s["name"]

            seq += 1
            self.subjects.append(subj)
        assert seq <= 255

        cur.execute("SELECT DISTINCT sub.code as subcode, tea.code as teacode, sub.name, auth_user.first_name, auth_user.last_name "
                    "FROM friprosveta_subject sub, friprosveta_activity act1, friprosveta_lecturetype lectype, timetable_activity act2, "
                    "     timetable_activityset actset, timetable_activityrealization actrea, timetable_activityrealization_teachers actreatea, "
                    "     timetable_teacher tea, auth_user "
                    "WHERE     sub.id = act1.subject_id "
                    "AND act1.lecture_type_id = lectype.id "
                    "AND lectype.short_name IN ('P', 'SEM') "  # filter lectures
                    "AND act1.activity_ptr_id = act2.id "
                    "AND act2.activityset_id = actset.id "
                    "AND actset.name = 'FRI 2014/2015, letni semester' "  # semester filter
                    "AND act2.id = actrea.activity_id "
                    "AND actrea.id = actreatea.activityrealization_id "
                    "AND actreatea.teacher_id = tea.id "
                    "AND tea.user_id = auth_user.id;")
        res_lectures = cur.fetchall()
        for l in res_lectures:
            # ugly hack
            if l["name"].startswith("Zagovori") or l["name"].startswith("Seminar"):
                continue
            subj = self.get_by_original_id(ConvertedResult.SUBJECT, l["subcode"])
            subj.professors.append(l["teacode"])

        cur.execute("SELECT DISTINCT sub.code as subcode, tea.code as teacode, sub.name, auth_user.first_name, auth_user.last_name "
                    "FROM friprosveta_subject sub, friprosveta_activity act1, friprosveta_lecturetype lectype, timetable_activity act2, "
                    "     timetable_activityset actset, timetable_activityrealization actrea, timetable_activityrealization_teachers actreatea, "
                    "     timetable_teacher tea, auth_user "
                    "WHERE     sub.id = act1.subject_id "
                    "AND act1.lecture_type_id = lectype.id "
                    "AND lectype.short_name IN ('LV', 'AV', 'lab.', 'TUT') "  # filter lectures
                    "AND act1.activity_ptr_id = act2.id "
                    "AND act2.activityset_id = actset.id "
                    "AND actset.name = 'FRI 2014/2015, letni semester' "  # semester filter
                    "AND act2.id = actrea.activity_id "
                    "AND actrea.id = actreatea.activityrealization_id "
                    "AND actreatea.teacher_id = tea.id "
                    "AND tea.user_id = auth_user.id;")
        res_tutorials = cur.fetchall()
        for l in res_tutorials:
            # ugly hack
            if l["name"].startswith("Zagovori") or l["name"].startswith("Seminar"):
                continue
            subj = self.get_by_original_id(ConvertedResult.SUBJECT, l["subcode"])
            subj.assistants.append(l["teacode"])

        # HACK: if there are no assistants, copy over professors
        # HACK: if there are not professors, copy over assitants
        for subj in self.subjects:
            if len(subj.assistants) == 0:
                subj.assistants = subj.professors
            if len(subj.professors) == 0:
                subj.professors = subj.assistants

        # get subject classrooms
        cur.execute("SELECT s.code, subject_id, classroom_id, s.name, c.\"shortName\""
                    "FROM (SELECT sub.id, count(DISTINCT actreq.resource_id) AS subject_requirement_count "
                    "      FROM friprosveta_subject sub, friprosveta_activity act1, timetable_activity act2, timetable_activity_requirements actreq "
                    "      WHERE     sub.id = act1.subject_id "
                    "            AND act1.activity_ptr_id = act2.id "
                    "            AND actreq.activity_id = act2.id "
                    "      GROUP BY sub.id) AS subjectreqs, "
                    "      (SELECT subject_id, classroom_id, count(*) AS classroom_subject_offer_count "
                    "      FROM (SELECT sub.id AS subject_id, actreq.resource_id AS subject_resource_id "
                    "          FROM friprosveta_subject sub, friprosveta_activity act1, timetable_activity act2, timetable_activity_requirements actreq "
                    "          WHERE     sub.id = act1.subject_id "
                    "                AND act1.activity_ptr_id = act2.id "
                    "                AND actreq.activity_id = act2.id) AS subjectres, "
                    "      (SELECT c.id AS classroom_id, c.\"shortName\", res.resource_id AS classroom_resource_id "
                    "      FROM timetable_classroom c, timetable_classroomset cs, timetable_classroomset_classrooms csc, timetable_classroomnresources res "
                    "      WHERE     c.id = csc.classroom_id "
                    "            AND cs.id = csc.classroomset_id "
                    "            AND c.id = res.classroom_id "
                    "            AND cs.name = 'FRI rooms & velika fizikalna predavalnica') AS classroomres "
                    "      WHERE subjectres.subject_resource_id = classroomres.classroom_resource_id "
                    "      GROUP BY subjectres.subject_id, classroomres.classroom_id) AS classroom_subject_offers, "
                    "      timetable_classroom c, "
                    "      friprosveta_subject s "
                    "WHERE     subjectreqs.id = classroom_subject_offers.subject_id "
                    "      AND subjectreqs.subject_requirement_count = classroom_subject_offers.classroom_subject_offer_count "
                    "      AND c.id = classroom_id "
                    "      AND s.id = subject_id "
                    "ORDER BY name, subject_id, \"shortName\"")
        res_subject_classrooms = cur.fetchall()

        subject_classrooms = {}
        for tup in res_subject_classrooms:
            if tup["code"] not in subject_classrooms:
                subject_classrooms[tup["code"]] = []
            subject_classrooms[tup["code"]].append(tup["classroom_id"])

        # get the subject student counts to see which classrooms can be used for lectures
        subject_student_counts = {}
        for stud in self.students:
            for sub in stud.subjects:
                if sub not in subject_student_counts:
                    subject_student_counts[sub] = 0
                subject_student_counts[sub] += 1

        # assign lecture classrooms and tutorial classrooms to each subject
        for subj in self.subjects:
            subject_student_count = subject_student_counts[subj.original_id]
            assert subject_student_count is not None and subject_student_count > 0

            possible_lecture_classrooms = []
            possible_tutorial_classrooms = []

            # all classrooms above or equal to the student count are eligible to be the lecture classroom
            # fizika special case: it's only done in the one lecture room
            if "fizika" in subj.descriptive.lower():
                possible_lecture_classrooms.append(self.get_by_original_id(ConvertedResult.CLASSROOM, "97"))
            else:
                for clrm in self.classrooms:
                    # skip the fizika-only classroom
                    if clrm.original_id == "97":
                        continue
                    if clrm.lecture_capacity >= subject_student_count:
                        possible_lecture_classrooms.append(clrm.original_id)

            # if there are no available classrooms, only use the largest one
            if len(possible_lecture_classrooms) == 0:
                largest = self.classrooms[0]
                for crum in self.classrooms:
                    if crum.lecture_capacity > largest.lecture_capacity and "fizika" not in crum.descriptive.lower():
                        largest = crum
                possible_lecture_classrooms.append(largest.original_id)


            # process tutorial classrooms
            # classrooms larger than 50 aren't eligible by default
            # if there are no limitations, everything is eligible
            # otherwise only the ones specified
            if subj.original_id not in subject_classrooms:
                # everything eligible
                for clrm in self.classrooms:
                    # this also skips VFP, so the special case isn't needed
                    if clrm.tutorial_capacity > 50:
                        continue
                    possible_tutorial_classrooms.append(clrm.original_id)
            else:
                # only specified
                for subject_code, classrooms in subject_classrooms.items():
                    possible_tutorial_classrooms = classrooms

            subj.lecture_classrooms = possible_lecture_classrooms
            subj.tutorial_classrooms = possible_tutorial_classrooms

    def export_classrooms(self, location, name):
        clrm_root = ET.Element("classrooms")
        for clrm in self.classrooms:
            print(clrm.descriptive)
            clrm_element = ET.SubElement(clrm_root, "classroom")
            clrm_element.set("id", str(clrm.generated_id))

            lecture_capacity_element = ET.SubElement(clrm_element, "lecture_capacity")
            lecture_capacity_element.text = str(clrm.lecture_capacity)

            tutorial_capacity_element = ET.SubElement(clrm_element, "tutorial_capacity")
            tutorial_capacity_element.text = str(clrm.tutorial_capacity)
        save_pretty(clrm_root, location, name)

    def export_professors(self, location, name):
        prof_root = ET.Element("professors")
        for prof in self.professors:
            prof_element = ET.SubElement(prof_root, "professor")
            prof_element.set("id", str(prof.generated_id))

            name_element = ET.SubElement(prof_element, "name")
            name_element.text = str(prof.name)

            hours_element = ET.SubElement(prof_element, "available_hours")
            hours_element.text = str(prof.available_hours)
        save_pretty(prof_root, location, name)

    def export_subjects(self, location, name):
        subj_root = ET.Element("subjects")
        for subj in self.subjects:
            subj_element = ET.SubElement(subj_root, "subject")
            subj_element.set("id", str(subj.generated_id))

            lecture_classrooms_container = ET.SubElement(subj_element, "lecture_classrooms")
            for s in subj.lecture_classrooms:
                clrm = self.get_by_original_id(ConvertedResult.CLASSROOM, s)
                id_element = ET.SubElement(lecture_classrooms_container, "id")
                id_element.text = str(clrm.generated_id)

            tutorial_classrooms_container = ET.SubElement(subj_element, "tutorial_classrooms")
            for s in subj.tutorial_classrooms:
                clrm = self.get_by_original_id(ConvertedResult.CLASSROOM, s)
                id_element = ET.SubElement(tutorial_classrooms_container, "id")
                id_element.text = str(clrm.generated_id)

            professors_container = ET.SubElement(subj_element, "professors")
            for s in subj.professors:
                prof = self.get_by_original_id(ConvertedResult.PROFESSOR, s)
                id_element = ET.SubElement(professors_container, "id")
                id_element.text = str(prof.generated_id)

            assistants_container = ET.SubElement(subj_element, "assistants")
            for s in subj.assistants:
                ass = self.get_by_original_id(ConvertedResult.PROFESSOR, s)
                id_element = ET.SubElement(assistants_container, "id")
                id_element.text = str(ass.generated_id)
        save_pretty(subj_root, location, name)

    def export_students(self, location, name):
        stud_root = ET.Element("students")
        for stud in self.students:
            stud_element = ET.SubElement(stud_root, "student")
            stud_element.set("id", str(stud.generated_id))

            lecture_classrooms_container = ET.SubElement(stud_element, "subjects")
            for s in stud.subjects:
                subj = self.get_by_original_id(ConvertedResult.SUBJECT, s)
                id_element = ET.SubElement(lecture_classrooms_container, "id")
                id_element.text = str(subj.generated_id)
        save_pretty(stud_root, location, name)


if __name__ == '__main__':
    import psycopg2
    import psycopg2.extras
    import xml.etree.ElementTree as ET
    from xml.dom import minidom
    from random import shuffle
    import pickle

    EXPORT_PATH = "gen/"

    conn = psycopg2.connect("dbname=urnik user=urnik")
    cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)

    result = ConvertedResult()

    result.get_students(cur)
    result.get_classrooms(cur)
    result.get_professors(cur)
    result.get_subjects(cur)

    # do some hacks
    result.filter()
    result.check_uniqueness()

    result.export_students(EXPORT_PATH, "students.xml")
    result.export_classrooms(EXPORT_PATH, "classrooms.xml")
    result.export_professors(EXPORT_PATH, "professors.xml")
    result.export_subjects(EXPORT_PATH, "subjects.xml")

    result.save(EXPORT_PATH)

    cur.close()
    conn.close()

