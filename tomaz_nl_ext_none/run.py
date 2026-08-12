import json
import subprocess
import os

BENCHMARK = "/home/tomaz/Projects/cvc5/reject-mv/tomaz_nl_ext_none/QF_NRA"
TIMEOUT = 300

def run_cvc5(p, data):
    flags = ["--nl-ext=none", "--nl-cov-force"]
    cvc5_bin = "/home/tomaz/Projects/cvc5/reject-mv/build/bin/cvc5"
    try:
        out = subprocess.run([cvc5_bin, p] + flags, capture_output=True, text=True, timeout=TIMEOUT)
        data[p]["answer"] = out.stdout
        data[p]["status"] = "PROCESSED"
        data[p]["time_taken"] = TIMEOUT - 1
    except:
        data[p]["status"] = "TIMEOUT"
    with open("data.json", "w") as f:
        json.dump(data, f)

def main():
    with open("data.json", "r") as f:
        data = json.load(f)

    total = 0
    for root, _, files in os.walk(BENCHMARK):
        for file in files:
            fp = root + "/" + file
            if data[fp]["status"] == "TIMEOUT" and data[fp]["sat?"] == "unsat":
                total += 1
    done = 0
    for root, _, files in os.walk(BENCHMARK):
        for file in files:
            fp = root + "/" + file
            if data[fp]["status"] == "TIMEOUT" and data[fp]["sat?"] == "unsat":
                print("processing", fp)
                run_cvc5(fp, data)
                done += 1
                print("done", done, "/", total)

if __name__ == '__main__':
    main()
