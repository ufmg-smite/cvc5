import json

with open("data.json", "r") as f:
    data = json.load(f)
    processed = 0
    univ = 0
    unsat_timeout = 0
    done_in_preprocessing = 0
    known_unsat = 0
    for k, v in data.items():
        if v["status"] == "TIMEOUT" and v["sat?"] == "unsat":
            print(k)
        if v["sat?"] == "unsat":
            known_unsat += 1
        if v["status"] == "PROCESSED" and "sat" in v["answer"] and v["sat?"] == "unsat":
            done_in_preprocessing += 1
        if v["status"] == "PROCESSED" and v["sat?"] == "unsat":
            processed += 1
            if "IS NOT UNIVARIATE" in v["answer"]:
                univ += 1
        if v["status"] == "TIMEOUT" and v["sat?"] == "unsat":
            unsat_timeout += 1
    # print("processed =", processed)
    # print("univ =", univ)
    # print("unsat timeout =", unsat_timeout)
    # print("done in preprocessing =", done_in_preprocessing)
    # print("known unsat =", known_unsat)
