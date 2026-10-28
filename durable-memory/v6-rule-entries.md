# v6 shop entry dispatch

SDK and suite pins: lmcode-cpp d608ba2. scripts/sync.py imported committed SDK bytes and the clean canonical 94-case suite. No expected result was regenerated or edited in this port.

main.cpp runs the body, then source-order item/total entries. Item arguments are the existing C arena Money pointers; total_handle calls the existing Shop.total binding, which delegates to sample_shop_total. Returned values use the reference's formatting. entries_called counts calls independently of ledger records. Exceptions are caught on the C++ side as run-phase errors; no exception or VM API crosses into domain.c. Domain allocation/arithmetic and all old callbacks are unchanged.

A clean pre-dispatch build failed 068 (seven missing calls) and 069 (four missing value lines). A second make clean followed by the full test.sh passed the C11 domain tests, all 94/94 exact-output cases, 94/94 compiler reproduction, compile-error checks and nine runner tests. The v5 rejection fixture passes.

Local compiler invocation: LMSCRIPT_BIN=/home/mma/repos/lmctl-mono-v6entries/lmscript/bin/lmscript LMSCRIPT_MIN=0. The task targets mono main 56f19153, while the supplied read-only checkout reports 0e621804. The supplied binary reproduced all 94 files; it was not rebuilt or edited. No global compiler link/install, release or CI dispatch was used.
