#!/usr/bin/env python3
"""Offline manifest/side-effect controls for the sample host's dry mode."""
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HOST = Path(sys.argv[1]).resolve()
PROGRAM = ROOT / "sample-suite/bytecode/024-file-functions.dslb"


def invoke(directory, dry):
    part = str(PROGRAM.resolve())
    manifest = directory / "invocation.json"
    manifest.write_text(json.dumps({
        "version": 1,
        "mode": "single",
        "dry": dry,
        "parts": [{"index": 0, "file": part}],
    }))
    env = dict(os.environ)
    env["LMCODE_INVOCATION_FILE"] = str(manifest)
    env["SAMPLE_TMP"] = str(directory)
    return subprocess.run([str(HOST), "--lmcode", part], capture_output=True, env=env)


def frame(stdout):
    lines = stdout.splitlines()
    starts = [i for i, line in enumerate(lines) if line == b"LMCODE_RETURN_BEGIN"]
    ends = [i for i, line in enumerate(lines) if line == b"LMCODE_RETURN_END"]
    assert len(starts) == len(ends) == 1 and ends[0] == starts[0] + 2, stdout
    return json.loads(lines[starts[0] + 1])


with tempfile.TemporaryDirectory(prefix="lmcode-h6-dry-") as tmp:
    directory = Path(tmp)
    result = invoke(directory, True)
    envelope = frame(result.stdout)
    sentinel = directory / "ledger.txt"
    assert not sentinel.exists(), ("dry effect callback wrote sentinel", result.returncode, result.stdout)
    assert result.returncode == 3, (result.returncode, result.stdout, result.stderr)
    assert envelope.get("dry") is True and envelope.get("ok") is False, envelope
    print("PASS dry manifest skips file-write sentinel and marks failure")

with tempfile.TemporaryDirectory(prefix="lmcode-h6-real-") as tmp:
    directory = Path(tmp)
    result = invoke(directory, False)
    envelope = frame(result.stdout)
    sentinel = directory / "ledger.txt"
    assert result.returncode == 0 and sentinel.read_bytes() == b"onetwo", (result.returncode, result.stdout, result.stderr)
    assert "dry" not in envelope and envelope["ok"] is True, envelope
    print("PASS real manifest preserves file effect and omits dry marker")
