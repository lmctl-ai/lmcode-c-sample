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
operator callbacks, error cases, and the complete 72-function standard catalog.
`sample-suite/sample.dsl.json` is the exact schema used for bytecode binding.
The suite is pinned to SDK commit `9a33f9bd139518f7ee02fd19d27c3b895c079c38`;
the C SDK archive is vendored under `include/`, `stdlib/`, and `src/lmcode_c.cpp`.

The C side uses opaque registry/context/program handles. Domain callbacks use
length-counted byte strings, typed object handles, and stable host-owned Money
objects. Standard lists/maps are created and mutated through
`lmcode_registry_call_function` / `lmcode_registry_call_method`, so collection
semantics remain in the SDK. Their result strings are borrowed until the next
registry call and their object pointers remain valid for the VM run.

Protocol commands are `--version`, `--dsl-schema [--json]`,
`--dsl-signatures`, and `--dsl FILE`. Program output is one JSON result line;
CLI/load/run/application errors use exit codes 2/3/4. The suite runs all 66
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
| std list/map | registry-owned opaque pointer | Use generic C registry calls; pointers expire between VM runs. |
| void | no C result | Callback returns `LMCODE_VOID`. |

Callback arguments are borrowed for the callback. Callback-returned strings and
type names must remain valid through callback return; generic call results are
borrowed until the next registry call. Domain objects are owned by `SampleHost`.

Adding a C host function means adding its schema entry and one descriptor in
`sample_host_create`; no C++ domain code is needed. The canonical suite and
expected outputs are never regenerated in this port.

Implementation map: `src/shop.h` defines the opaque-domain structs and ownership;
`src/shop.c` registers all host signatures and dispatches C callbacks;
`src/main.cpp` implements only CLI discovery, schema loading, VM invocation and
JSON serialization; `include/lmcode_c.h` is the C boundary; `scripts/sync.py`
checks the immutable SDK/suite locks. `test.sh` runs lock verification, the
native C11/C++17 build, protocol checks, and the exact corpus.
