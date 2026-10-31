# Invocation envelopes (R6)

One shared Host/context/ledger per process invocation; repeated --lmcode pairs
are one ordered collector. --first/--last role pairs reorder dispatch independently
of add indices. Each slot owns a sink retained through body and selected entries;
complete validates/seals. First failure retains its slot; final negative ledger is
global app failure. Failures carry no result/parts. Preserve entry/HALT diagnostics.
Success prints one tagged host_summary, then one compact terminal envelope and LF.
No aliases, old unframed success, per-part frames, deadline or retry.

SDK and canonical suite are refreshed through own sync scripts. Compile fixtures
with sample-suite/compile-fixture.mjs (compiler library only); public examples use
run_lmcode, never name bytecode. The 123 old traces remain covered with refreshed
envelopes. Real builder tests cover duplicate/reused/one-slot programs, reverse
roles/shared ledger, entry/global failures and retained summaries. Compiled mutants
in sample-suite/mutate-invocations.py kill lost summary/reset context/wrong slot;
a real subprocess-per-file wrapper also fails.
