import json
import pickle

from database_data_convert import *

mapping_file = "gen/data.pickle"
timetable_file = "out/saved/three-all-5000-eth-REAL-timetable.json"
output_file = "out/timetable.json"

if __name__ == "__main__":
    with open(timetable_file, "r") as f:
        timetable_contents = f.read()
    timetable = json.loads(timetable_contents, encoding="UTF-8")

    with open(mapping_file, "rb") as f:
        cr = pickle.load(f, encoding="UTF-8")

    entries = timetable["timetable_entries"]
    for e in entries:
        e["classroom"] = cr.get_by_generated_id(ConvertedResult.CLASSROOM, e["classroom"]).descriptive
        e["subject"] = cr.get_by_generated_id(ConvertedResult.SUBJECT, e["subject"]).descriptive

        new_students = []
        for s in e["students"]:
            new_students.append(cr.get_by_generated_id(ConvertedResult.STUDENT, s).descriptive)
        e["students"] = new_students

        new_professors = []
        for p in e["professors"]:
            new_professors.append(cr.get_by_generated_id(ConvertedResult.PROFESSOR, p).descriptive)
        e["professors"] = new_professors

    with open(output_file, "w") as f:
        f.write(json.dumps(timetable, sort_keys=True))
