#!/usr/bin/env bash
set -euo pipefail
python3 -B scripts/sync.py check
make test
python3 -B sample-suite/audit.py --std std/lmcode-std.schema.json
python3 -B sample-suite/check-recompile.py
python3 -B sample-suite/test_runner.py
