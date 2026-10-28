# Wrap a C library with lmcode-cpp

This sample keeps the shop library in C11 and embeds lmcode with a small C++17
app. Start with [domain.h](src/domain.h) and [domain.c](src/domain.c): they know
nothing about the VM. [bindings.cpp](src/bindings.cpp) converts SDK values and
calls that library directly; [main.cpp](src/main.cpp) handles the CLI and results.

```sh
./test.sh                         # locks, standalone C test, complete corpus
make test-domain                  # C11 library only; no C++ or SDK linkage
./build/lmcode-c-sample --dsl-schema --json
./build/lmcode-c-sample --dsl sample-suite/bytecode/001-literals.dslb
```

## Wrap a C library

1. Define business structs and functions in `domain.h`; expose them to C++ with
   `extern "C"`. Keep allocation, Money arithmetic, Shop totals and Ledger
   mutation in `domain.c`. Its arena owns stable Money pointers until teardown.
2. Describe those objects in [host.schema.json](sample-suite/host.schema.json).
   The canonical merged [sample.dsl.json](sample-suite/sample.dsl.json) includes
   all 181 standard signatures; the teaching host contributes 45 signatures.
3. In `bindings.cpp`, register functions, context objects and overloaded methods
   on `dslvm::Registry`. Call the C functions, translate nonzero error returns
   into `dslvm::Error` on the C++ side (no exceptions cross the C boundary),
   and return `dslvm::Value` objects. This is an application
   binding, with no generic C wrapper around the VM.
4. Keep the C domain alive throughout `Program::run`; destroy it afterward.
   `main.cpp` loads bytecode with the exact schema and serializes the final ledger.
5. Run the canonical corpus without regenerating its expected output in this port.
   The [coverage matrix](sample-suite/README.md) documents the feature programs.

## Conversions

| lmcode type | C library / C++ binding | Rule |
|---|---|---|
| int | `int64_t` | Wrap via unsigned arithmetic; never narrow to C `int`. |
| float | `double` | Preserve binary64, signed zero, NaN, infinities and subnormals. Money rejects nonfinite/out-of-range prices. |
| bool | `int` / SDK `bool` | Convert false/true to zero/one at a C boundary. |
| string | `const char *` + `size_t` / `std::string` | Bytes, including NUL and invalid UTF-8; use explicit lengths. |
| enum | SDK string | Preserve the declared member spelling (`USD`/`EUR`). |
| object | C struct pointer / typed SDK handle | Domain owns stable pointers; typed null differs from void. |
| list/map | SDK registry-owned collection | Conversion helpers copy inputs into new arena collections; never free SDK objects in C. |
| void | no C result / `Value::none()` | Do not invent a null object for a void method. |

SDK arguments are borrowed during callbacks; `Value::string` owns its returned
bytes. `sample_ledger_note` copies bytes into the C domain. Standard collections
expire between VM runs. Domain pointers remain valid until `sample_domain_destroy`.

## Protocol and reproducibility

Discovery supports `--version`, `--dsl-schema [--json]`, and `--dsl-signatures`.
`--dsl FILE` emits program print bytes followed by one JSON result line. Exit
codes are success 0, CLI/load 2, runtime 3, and application failure 4. Errors retain
source line/column. Set `SAMPLE_TMP` for file programs; the suite sets an isolated
directory automatically. File functions access native paths without a sandbox.

`./test.sh` runs all 91 cases against exact stdout bytes and exit codes, including
invalid bytecode/schema/handle imports. [sdk.lock](sdk.lock) and
[suite.lock](suite.lock) identify the immutable SDK and canonical corpus snapshots;
`scripts/sync.py check` verifies every vendored byte. Linux, macOS and Windows CI
build the same C11 library plus C++17 app. No separate C SDK is required.

## Refreshing the rule-method corpus

The canonical suite is pinned to `lmcode-cpp` commit `aec9b09`. Program 067
calls an `@PRICE_RULE` function implementing the schema SAM `Adjuster`; the
shared negative compiler case rejects a mismatched signature. This metadata is
compiler-only: the C library and C++ VM still execute DSLB v5. Schema bytes and
all pinned programs move together. The SDK pin also includes std-v3 codecs and
map snapshots used by the current canonical suite.

```sh
python3 scripts/sync.py sdk ../lmcode-cpp
python3 scripts/sync.py suite ../lmcode-cpp
LMSCRIPT_BIN=/path/to/rule-method-capable/lmscript ./test.sh
```

Use an R47/R48-capable compiler for source reproduction; the older published
compiler default in CI must be overridden when a matching release is available.
