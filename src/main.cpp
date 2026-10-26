#include "bindings.hpp"
#include "dslvm.hpp"
#include "sample_schema.hpp"
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
static std::string bytes(const char *path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("cannot read sample input");
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}
static int usage() {
  std::cout << "{\"ok\": false, \"phase\": \"cli\", \"code\": \"CLI_USAGE\", "
               "\"message\": \"usage: --version | --dsl-schema [--json] | "
               "--dsl-signatures | --dsl FILE\"}\n";
  return 2;
}
int main(int argc, char **argv) {
  std::cout.imbue(std::locale::classic());
  sample::Host host;
  if (argc == 2 && std::string(argv[1]) == "--version") {
    std::cout << "lmcode-c-sample 0.3.0\n";
    return 0;
  }
  if ((argc == 2 || (argc == 3 && std::string(argv[2]) == "--json")) &&
      std::string(argv[1]) == "--dsl-schema") {
    std::cout.write(sample::schema, sizeof(sample::schema) - 1);
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "--dsl-signatures") {
    for (const auto &s : host.registry.signatures())
      std::cout << s << '\n';
    return 0;
  }
  if (argc != 3 || std::string(argv[1]) != "--dsl")
    return usage();
  std::string bytecode;
  try {
    bytecode = bytes(argv[2]);
  } catch (...) {
    return dslvm::report_app_failure("cannot read sample input");
  }
  std::string schema(sample::schema, sizeof(sample::schema) - 1);
  dslvm::Program program;
  try {
    program = dslvm::Program::load(bytecode, schema, host.registry);
  } catch (const dslvm::Error &e) {
    std::string m = e.code == "LOAD_SCHEMA_MISMATCH" ? "sample schema mismatch"
                    : e.code == "LOAD_UNBOUND"       ? "sample import mismatch"
                                               : "invalid sample bytecode";
    return dslvm::report_error("load", dslvm::Error(e.code, m));
  }
  auto context = host.context();
  try {
    program.run(context, host.registry);
  } catch (const dslvm::Error &e) {
    return dslvm::report_error("run", e);
  }
  if (host.domain.ledger.balance < 0)
    return dslvm::report_app_failure("ledger: negative balance");
  std::string note =
      host.domain.ledger.note
          ? std::string(host.domain.ledger.note, host.domain.ledger.note_length)
          : std::string();
  return dslvm::report_ok(
      "{\"balance\": " + std::to_string(host.domain.ledger.balance) +
      ", \"entries\": " + std::to_string(host.domain.ledger.entries) +
      ", \"note\": " + dslvm::json_escape(note) + "}");
}
