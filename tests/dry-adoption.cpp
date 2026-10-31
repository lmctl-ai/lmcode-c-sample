#include "bindings.hpp"
#include "sample_schema.hpp"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
std::string read_bytes(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  std::ostringstream bytes;
  bytes << input.rdbuf();
  if (!input && !input.eof()) throw std::runtime_error("cannot read dry sentinel bytecode");
  return bytes.str();
}
void sample_tmp_path(const std::string& path) {
#ifdef _WIN32
  if (_putenv_s("SAMPLE_TMP", path.c_str()) != 0) throw std::runtime_error("cannot set SAMPLE_TMP");
#else
  if (setenv("SAMPLE_TMP", path.c_str(), 1) != 0) throw std::runtime_error("cannot set SAMPLE_TMP");
#endif
}
}

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto base = std::filesystem::temp_directory_path() / ("lmcode-h6-adoption-" + std::to_string(nonce));
  const auto bytecode = read_bytes(argv[1]);
  const std::string schema(sample::schema, sizeof(sample::schema) - 1);

  const auto dry_dir = base / "dry";
  std::filesystem::create_directories(dry_dir);
  sample_tmp_path(dry_dir.string());
  {
    sample::Host host;
    auto* sample_tmp = host.registry.function_by_signature("fn sample_tmp() -> string");
    assert(sample_tmp && sample_tmp->fn && sample_tmp->dry_fn);
    std::ostringstream diagnostics;
    host.registry.set_std_output(&diagnostics);
    auto program = dslvm::Program::load(bytecode, schema, host.registry);
    auto context = host.context();
    context.set_dry(true);
    try {
      program.run(context, host.registry);
      assert(false && "read after the skipped write should fail on the missing sentinel");
    } catch (const dslvm::Error& error) {
      assert(error.code == "RUN_HOST");
    }
    assert(host.sample_tmp_calls == 1);
    assert(!std::filesystem::exists(dry_dir / "ledger.txt"));
  }

  const auto real_dir = base / "real";
  std::filesystem::create_directories(real_dir);
  sample_tmp_path(real_dir.string());
  {
    sample::Host host;
    auto* sample_tmp = host.registry.function_by_signature("fn sample_tmp() -> string");
    assert(sample_tmp && sample_tmp->fn && sample_tmp->dry_fn);
    std::ostringstream diagnostics;
    host.registry.set_std_output(&diagnostics);
    auto program = dslvm::Program::load(bytecode, schema, host.registry);
    auto context = host.context();
    program.run(context, host.registry);
    assert(host.sample_tmp_calls == 1);
    assert(std::filesystem::exists(real_dir / "ledger.txt"));
    std::ifstream input(real_dir / "ledger.txt", std::ios::binary);
    std::ostringstream bytes;
    bytes << input.rdbuf();
    assert(bytes.str() == "onetwo");
    assert(diagnostics.str() == "true:onetwo:1\n");
  }

  std::filesystem::remove_all(base);
  std::cout << "PASS offline dry sentinel, one preview read, and unchanged real callback\n";
}
