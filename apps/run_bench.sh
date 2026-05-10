#!/usr/bin/env bash
# Run the canonical bench matrix: preset x np x threads.
# Writes one JSON per (preset, np, threads) into docs/benchmarks/raw/.
set -euo pipefail

PRESETS="${PRESETS:-release}"
NP_LIST="${NP_LIST:-2 4 8}"
THREADS_LIST="${THREADS_LIST:-1}"
BATCH="${BATCH:-1000}"
WARMUP="${WARMUP:-50}"
REPETITIONS="${REPETITIONS:-3}"

OUT_DIR="docs/benchmarks/raw"
mkdir -p "$OUT_DIR"

for preset in $PRESETS; do
    for np in $NP_LIST; do
        for threads in $THREADS_LIST; do
            for rep in $(seq 1 "$REPETITIONS"); do
                exe="build/${preset}/ccsd_bench"
                if [[ ! -x "$exe" ]]; then
                    cmake --preset "$preset" >/dev/null
                    cmake --build --preset "$preset" --target ccsd_bench >/dev/null
                fi
                json="${OUT_DIR}/${preset}_np${np}_t${threads}_rep${rep}.json"
                if [[ -n "${BIND_TO:-}" ]]; then
                    bind_args="--bind-to ${BIND_TO} --map-by socket:PE=${threads}"
                else
                    bind_args=""
                fi
                OMP_NUM_THREADS="$threads" \
                    mpirun --oversubscribe ${bind_args} -np "$np" "$exe" \
                        --batch "$BATCH" --warmup "$WARMUP" \
                        --report "$json"
                # Decorate with preset/threads/sha for the collator.
                python3 -c "
import json, subprocess, sys, os
p = sys.argv[1]
d = json.load(open(p))
d['preset'] = '${preset}' + ('-pinned' if '${BIND_TO:-}' else '')
d['threads_per_rank'] = ${threads}
d['repetition'] = ${rep}
d['git_sha'] = subprocess.check_output(['git','rev-parse','--short','HEAD']).decode().strip()
d['host'] = os.uname().nodename
json.dump(d, open(p,'w'), indent=2)
" "$json"
            done
        done
    done
done

python3 apps/plot_or_table.py "$OUT_DIR" docs/benchmarks/pass-2-results.md
echo "Wrote docs/benchmarks/pass-2-results.md"
