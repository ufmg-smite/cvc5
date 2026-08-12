import json
import os

BENCHMARK = "/home/tomaz/Projects/cvc5/reject-mv/tomaz_nl_ext_none/QF_NRA"

data = dict()
for r, _, fs in os.walk(BENCHMARK):
    for f in fs:
        fp = r + "/" + f
        with open(fp) as problem_file:
            answer = "unknown"
            for line in problem_file:
                if ":status unsat" in line:
                    answer = "unsat"
                if ":status sat" in line:
                    answer = "sat"
        data[fp] = { "status": "UNPROCESSED", "sat?": answer }

with open("data.json", "w") as f:
    json.dump(data, f)
