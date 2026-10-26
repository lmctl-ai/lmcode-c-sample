# lmcode C shop sample

This is the C reference host for the shared lmcode teaching corpus. Read
`src/shop.h`, `src/shop.c`, and `src/main.cpp` first: the domain is pure C;
the C++ file only loads bytecode, serializes protocol output, and owns no domain
conversion logic.

```sh
make test
```

The host contract is the vendored `sample-suite/host.schema.json`: `Shop`,
`Items`, `Ledger`, and `Money`, `Currency`, all overloads, void methods,
operator callbacks, error cases, and the complete 171-function standard catalog.
`sample-suite/sample.dsl.json` is the exact schema used for bytecode binding.
The suite is pinned to SDK commit `e9a34f2b3280a8862f391dc18d5c45a7ef1605c5`;
the pinned SDK runtime is vendored under `include/`, `stdlib/`, and `std/`.

The C side is a standalone domain library in `src/domain.c/.h`: Money, Shop,
Items and Ledger state, wrapping, allocation and error-safe mutation live there.
`src/bindings.cpp` is the direct dslvm host binding; `src/main.cpp` only handles
CLI discovery, bytecode loading, protocol serialization and VM execution.
Standard collections stay in the SDK registry arena.

Protocol commands are `--version`, `--dsl-schema [--json]`,
`--dsl-signatures`, and `--dsl FILE`. Program output is one JSON result line;
CLI/load/run/application errors use exit codes 2/3/4. The suite runs all 77
canonical programs and diffs exact stdout and exit status.

## Conversions

| lmcode value | C representation | Boundary rule |
|---|---|---|
| int | `int64_t` | Two's-complement wrapping; never narrow to C `int`. |
| float | `double` | Preserve binary64 NaN, infinities, signed zero and subnormals. |
| bool | `int` | Only zero/one is interpreted as false/true. |
| string | `const char *` + `size_t` | Raw bytes, including NUL/invalid UTF-8; never use `strlen`. |
| enum | length-counted string | Preserve exact schema member spelling. |
| object | schema type + `void *` | Opaque stable handle; typed null is distinct from void. |
| std list/map | SDK registry-owned pointer | Bind through the SDK arena; pointers expire between VM runs. |
| void | no C result | Callback returns `LMCODE_VOID`. |

Callback arguments are borrowed for the callback. Callback-returned strings and
type names must remain valid through callback return; generic call results are
borrowed until the next SDK call. Domain objects are owned by `SampleDomain`.

Adding a C host function means adding its schema entry and one descriptor in
`sample_host_create`; no C++ domain code is needed. The canonical suite and
expected outputs are never regenerated in this port. The v2 SDK supplies 171
standard signatures; the teaching host contributes 45 host signatures.

Implementation map: `src/shop.h` defines the opaque-domain structs and ownership;
`src/shop.c` registers all host signatures and dispatches C callbacks;
`src/main.cpp` implements only CLI discovery, schema loading, VM invocation and
JSON serialization; `src/bindings.hpp` is the direct C++ host binding; `scripts/sync.py`
checks the immutable SDK/suite locks. `test.sh` runs lock verification, the
native C11/C++17 build, protocol checks, and the exact corpus.
