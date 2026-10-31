# Service-free invocation results (consensus slice 6)

The native CLI uses the canonical C++ ResultSink/frame API even though the shop
model is C. Bind an owned sink before body execution, preserve it through all
item/total callbacks, and emit only after successful shop validation and the
existing diagnostic. Do not reset per entry or attach the sink to collection
arenas. Entry HALT values and C shop/ledger state stay independent. Windows
stdout is binary, and failed body/entry/app work produces no terminal frame.

Sources and expectations come from the exact committed canonical sample-suite;
never record an oracle from this port. Sync with `python3 scripts/sync.py suite
/path/to/own/lmcode-cpp` and the SDK mode, verify locks, and run ./test.sh with
LMSCRIPT_BIN pointing to a private mono main build at 0bc80277 or newer. Wrappers
use `_lmcode_ ... out=ARGV[0]` and no longer call savedsl. The pure-C domain test
remains part of the gate.
