#!/usr/bin/env bash
set -euo pipefail
python3 -B scripts/sync.py check
make test
