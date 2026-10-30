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
   all 182 standard signatures; the teaching host contributes 45 signatures.
3. In `bindings.cpp`, register functions, context objects and overloaded methods
   on `dslvm::Registry`. Call the C functions, translate nonzero error returns
   into `dslvm::Error` on the C++ side (no exceptions cross the C boundary),
   and return `dslvm::Value` objects. This is an application
   binding, with no generic C wrapper around the VM.
4. Keep the C domain alive throughout the body and all rule calls; destroy it afterward.
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
codes are success 0, CLI 2, load/runtime 3, and application failure 4. Errors retain
source line/column. Set `SAMPLE_TMP` for file programs; the suite sets an isolated
directory automatically. File functions access native paths without a sandbox.

`./test.sh` runs all 108 cases against exact stdout bytes and exit codes, including
invalid bytecode/schema/handle imports. [sdk.lock](sdk.lock) and
[suite.lock](suite.lock) identify the immutable SDK and canonical corpus snapshots;
`scripts/sync.py check` verifies every vendored byte. Linux, macOS and Windows CI
build the same C11 library plus C++17 app. No separate C SDK is required.

## Refreshing the rule-method corpus

The SDK and 108-case canonical suite pin lmcode-cpp main 9235d01 and require DSLB
v6. Rules annotated item are called once per shop Money handle; total rules are
called once with the existing Shop.total() implementation. Entries run after the
body, in source order; other annotations are ignored. Non-void returns print
entry NAME = VALUE. Entry failures remain run-phase errors, and application
validation runs after dispatch.

entries_called is the number of successful rule calls, separate from the ledger's
entries field. The C domain and its stable arena pointers outlive the body and all
entry calls. No rule knowledge or VM dependency was added to domain.c/domain.h.

```sh
python3 scripts/sync.py sdk ../lmcode-cpp
python3 scripts/sync.py suite ../lmcode-cpp
LMSCRIPT_BIN=/path/to/v6/lmscript LMSCRIPT_MIN=0 ./test.sh
```

Source reproduction needs a v6 rule-entry compiler, not the older published
0.2.426 compiler. Use the supplied read-only mono lmscript build for development;
[local verification notes](durable-memory/v6-rule-entries.md) record the exact
path and observed revision. Expectations remain the committed reference bytes.

## Runnable annotated interface example

[examples/rule-entries.lms](examples/rule-entries.lms) defines two `@item`
functions and one `@total` function, each with `implements Adjuster`. The
schema declares `Adjuster(Money) -> void`; the compiler checks the signature.
The body prints first, then the host calls entries by annotation. `@never`
demonstrates an entry that this host does not dispatch.

```sh
make
lmscript examples/rule-entries.lms build/rule-entries.dslb
build/lmcode-c-sample --dsl build/rule-entries.dslb
```

Use lmscript 0.2.444 (DSLB v6). The expected trace is pinned in
[sample-suite/expected/068-rule-entries.out](sample-suite/expected/068-rule-entries.out).
For a non-void interface and returned entry value, see
[069-rule-value-entry.lms](sample-suite/programs/069-rule-value-entry.lms).

## Automatic releases

Every push to `main` builds and tests Linux x64/arm64, macOS x64/arm64,
and Windows x64. All five jobs must pass the full suite, bytecode
reproduction and the public annotated interface example before publication.
Pull requests run the same checks without publishing.

The release workflow installs lmctl and lmscript from npm and publishes an immutable
`v0.4.<workflow run number>` release, marking it latest. Executables report
that same version; assets use the lmscript platform naming convention and
include `SHA256SUMS`, license and notices. Failed tests prevent publication.
A workflow rerun keeps an already published release unchanged.

```sh
lmscript install github:lmctl-ai/lmcode-c-sample@latest
```

## Tests driven by lmscript

```sh
npm install -g @lmctl-ai/lmctl
lmctl test.lms
# Equivalent executable:
lmscript test.lms
```

Both executables run `.lms` files; `lmctl` also accepts a URL ending in `.lms`.
These repository tests require the checkout and run from its root because they
use the pinned corpus and native sources. `./test.sh` is a convenience wrapper.
CI uses `lmscript --no-preflight test.lms` to execute the tests once.

The suite, protocol checks, signature coverage, compiler reproduction and
negative controls are orchestrated by `.lms` files in `sample-suite/`. A small
Node adapter preserves binary hashes and byte-exact subprocess output; it
contains no test assertions. Python remains for SDK lock synchronization and
build-time schema embedding, not test orchestration.

## Text templates

[examples/text-template.lms](examples/text-template.lms) renders ordinary text from a nested context with indexed lists, loops and boolean conditions. The standard `template_render(string, Json) -> string` function performs one pass; inserted values stay literal. Compile it with `lmscript --no-preflight examples/text-template.lms build/text-template.dslb`, then run the sample host with `--dsl build/text-template.dslb`.

The lmscript-driven suite runs 108 cases, including template success/error cases. See the [portable template contract](https://github.com/lmctl-ai/lmcode-cpp/blob/main/std/template.md).
