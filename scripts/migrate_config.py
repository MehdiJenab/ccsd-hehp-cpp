import json, sys
src = json.load(open(sys.argv[1]))
arr = src["config"]
out = {}
for entry in arr:
    out.update(entry)
json.dump(out, open(sys.argv[1], "w"), indent=2)
