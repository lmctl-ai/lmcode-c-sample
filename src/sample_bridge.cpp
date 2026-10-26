#include "sample_bridge.h"
#include "../include/dslvm.hpp"
#include "../stdlib/testing.hpp"
#include <memory>
#include <ostream>
#include <streambuf>
#include <string>
#include <vector>

struct c_print_buf : std::streambuf {
  sample_print_callback callback = nullptr;
  void *user = nullptr;
  int overflow(int c) override {
    if (c != EOF && callback) {
      const char byte = static_cast<char>(c);
      callback(user, &byte, 1);
    }
    return c;
  }
  std::streamsize xsputn(const char *data, std::streamsize size) override {
    if (callback && size > 0)
      callback(user, data, static_cast<size_t>(size));
    return size;
  }
  int sync() override { return 0; }
};
struct sample_registry {
  dslvm::Registry value;
  mutable std::vector<std::string> signatures;
  std::string code, message, file, result_data, result_type;
  std::unique_ptr<c_print_buf> print_buf;
  std::unique_ptr<std::ostream> print_stream;
  sample_registry(dslvm::StdMode mode) : value(mode) {}
};
struct sample_context {
  dslvm::Context value;
  std::string file, code, message;
};
struct sample_program {
  dslvm::Program value;
  const sample_registry *registry = nullptr;
  mutable std::string code, message;
};

static void clear_error(sample_error *e) {
  if (e)
    *e = {nullptr, nullptr, 0, 0, 0, 0};
}
static int bad_argument(sample_error *out) {
  static thread_local const char *code = "RUN_HOST";
  static thread_local const char *message = "invalid null argument";
  if (out)
    *out = {code, message, 0, 0, 0, 0};
  return 1;
}
static int unknown_error(sample_error *out) {
  static thread_local const char *code = "RUN_HOST";
  static thread_local const char *message =
      "unknown C++ exception in lmcode host adapter";
  if (out)
    *out = {code, message, 0, 0, 0, 0};
  return 1;
}
static int fill(sample_error *out, sample_registry *r, const dslvm::Error &e) {
  if (!out)
    return 1;
  r->code = e.code;
  r->message = e.what();
  *out = {r->code.c_str(), r->message.c_str(), e.line,
          e.column,        e.has_location,     e.has_column};
  return 1;
}
static int fill_registry(const sample_registry *r, sample_error *out,
                         const dslvm::Error &e) {
  if (!out)
    return 1;
  sample_registry *mutable_registry = const_cast<sample_registry *>(r);
  mutable_registry->code = e.code;
  mutable_registry->message = e.what();
  *out = {mutable_registry->code.c_str(),
          mutable_registry->message.c_str(),
          e.line,
          e.column,
          e.has_location,
          e.has_column};
  return 1;
}

static dslvm::Value to_cpp(const sample_value &v) {
  switch (v.kind) {
  case LMCODE_INT:
    return dslvm::Value::integer(v.i);
  case LMCODE_FLOAT:
    return dslvm::Value::real(v.f);
  case LMCODE_STRING:
    if (v.length && !v.data)
      throw dslvm::Error("RUN_HOST",
                         "callback returned a string with null data");
    return dslvm::Value::string(std::string(v.data ? v.data : "", v.length));
  case LMCODE_BOOL:
    return dslvm::Value::boolean(v.b != 0);
  case LMCODE_OBJECT:
    if (v.type_length && !v.type)
      throw dslvm::Error("RUN_HOST",
                         "callback returned an object with null type");
    return dslvm::Value::object(
        std::string(v.type ? v.type : "", v.type_length), v.object);
  default:
    return dslvm::Value::none();
  }
}
static sample_value from_cpp(const dslvm::Value &v) {
  sample_value out{};
  out.kind = static_cast<sample_value_kind>(v.kind);
  out.i = v.i;
  out.f = v.f;
  out.b = v.b;
  out.data = v.s.data();
  out.length = v.s.size();
  out.object = v.obj;
  out.type = v.type.data();
  out.type_length = v.type.size();
  return out;
}
static sample_value owned_value(sample_registry *r, const dslvm::Value &v) {
  sample_value out = from_cpp(v);
  r->result_data = v.s; r->result_type = v.type;
  if (v.kind == dslvm::Value::STR) { out.data = r->result_data.data(); out.length = r->result_data.size(); }
  if (v.kind == dslvm::Value::OBJ) { out.type = r->result_type.data(); out.type_length = r->result_type.size(); }
  return out;
}
static sample_error callback_error(std::string &code, std::string &message,
                                   const sample_error &e) {
  code = e.code ? e.code : "RUN_HOST";
  message = e.message ? e.message : "callback failed";
  return {code.c_str(), message.c_str(), e.line,
          e.column,     e.has_location,  e.has_column};
}

sample_registry *sample_registry_create(void) {
  try {
    return new sample_registry(dslvm::StdMode::Enabled);
  } catch (...) {
    return nullptr;
  }
}
sample_registry *sample_registry_create_without_std(void) {
  try {
    return new sample_registry(dslvm::StdMode::Disabled);
  } catch (...) {
    return nullptr;
  }
}
void sample_registry_destroy(sample_registry *r) { delete r; }

int sample_registry_function(sample_registry *r, const char *name,
                             const char *const *params, size_t count,
                             const char *returns, sample_function_callback cb,
                             void *user, sample_error *error) {
  clear_error(error);
  if (!r || !name || !returns || !cb || (count && !params))
    return bad_argument(error);
  try {
    std::vector<std::string> p;
    for (size_t i = 0; i < count; ++i) {
      if (!params[i])
        return bad_argument(error);
      p.emplace_back(params[i]);
    }
    r->value.function(name, p, returns, [cb, user](dslvm::Args &a) {
      std::vector<sample_value> av;
      for (auto &v : a)
        av.push_back(from_cpp(v));
      sample_value result{};
      sample_error e{};
      std::string code, msg;
      if (cb(user, av.data(), av.size(), &result, &e)) {
        auto ce = callback_error(code, msg, e);
        throw dslvm::Error(ce.code, ce.message);
      }
      return to_cpp(result);
    });
    return 0;
  } catch (const dslvm::Error &e) {
    return fill(error, r, e);
  } catch (const std::exception &e) {
    r->code = "RUN_HOST";
    r->message = e.what();
    if (error)
      *error = {r->code.c_str(), r->message.c_str(), 0, 0, 0, 0};
    return 1;
  } catch (...) {
    return unknown_error(error);
  }
}
int sample_registry_method(sample_registry *r, const char *type,
                           const char *name, const char *const *params,
                           size_t count, const char *returns,
                           sample_method_callback cb, void *user,
                           sample_error *error) {
  clear_error(error);
  if (!r || !type || !name || !returns || !cb || (count && !params))
    return bad_argument(error);
  try {
    std::vector<std::string> p;
    for (size_t i = 0; i < count; ++i) {
      if (!params[i])
        return bad_argument(error);
      p.emplace_back(params[i]);
    }
    r->value.method(type, name, p, returns,
                    [cb, user](void *self, dslvm::Args &a) {
                      std::vector<sample_value> av;
                      for (auto &v : a)
                        av.push_back(from_cpp(v));
                      sample_value result{};
                      sample_error e{};
                      std::string code, msg;
                      if (cb(user, self, av.data(), av.size(), &result, &e)) {
                        auto ce = callback_error(code, msg, e);
                        throw dslvm::Error(ce.code, ce.message);
                      }
                      return to_cpp(result);
                    });
    return 0;
  } catch (const dslvm::Error &e) {
    return fill(error, r, e);
  } catch (const std::exception &e) {
    r->code = "RUN_HOST";
    r->message = e.what();
    if (error)
      *error = {r->code.c_str(), r->message.c_str(), 0, 0, 0, 0};
    return 1;
  } catch (...) {
    return unknown_error(error);
  }
}
int sample_registry_context(sample_registry *r, const char *name,
                            const char *type, sample_error *error) {
  clear_error(error);
  if (!r || !name || !type)
    return bad_argument(error);
  try {
    r->value.context(name, type);
    return 0;
  } catch (const dslvm::Error &e) {
    return fill(error, r, e);
  } catch (const std::exception &e) {
    r->code = "RUN_HOST";
    r->message = e.what();
    if (error)
      *error = {r->code.c_str(), r->message.c_str(), 0, 0, 0, 0};
    return 1;
  } catch (...) {
    return unknown_error(error);
  }
}
size_t sample_registry_signature_count(const sample_registry *r) {
  if (!r)
    return 0;
  try {
    return r->value.signatures().size();
  } catch (...) {
    return 0;
  }
}
const char *sample_registry_signature(const sample_registry *r, size_t i) {
  if (!r)
    return nullptr;
  try {
    r->signatures = r->value.signatures();
    if (i >= r->signatures.size())
      return nullptr;
    return r->signatures[i].c_str();
  } catch (...) {
    return nullptr;
  }
}
int sample_registry_register_testing(sample_registry *r,
                                     sample_print_callback cb, void *user,
                                     sample_error *error) {
  clear_error(error);
  if (!r)
    return bad_argument(error);
  if (r->print_stream) {
    r->code = "LOAD_UNBOUND";
    r->message = "registry: testing functions are registered twice";
    if (error)
      *error = {r->code.c_str(), r->message.c_str(), 0, 0, 0, 0};
    return 1;
  }
  try {
    r->print_buf = std::make_unique<c_print_buf>();
    r->print_buf->callback = cb;
    r->print_buf->user = user;
    r->print_stream = std::make_unique<std::ostream>(r->print_buf.get());
    dslvm::stdlib::register_testing(r->value, r->print_stream.get());
    return 0;
  } catch (const dslvm::Error &e) {
    return fill(error, r, e);
  } catch (const std::exception &e) {
    r->code = "RUN_HOST";
    r->message = e.what();
    if (error)
      *error = {r->code.c_str(), r->message.c_str(), 0, 0, 0, 0};
    return 1;
  } catch (...) {
    return unknown_error(error);
  }
}
int sample_registry_call_function(sample_registry *r, const char *signature,
                                  const sample_value *values, size_t count,
                                  sample_value *result, sample_error *error) {
  clear_error(error);
  if (!r || !signature || (count && !values) || !result)
    return bad_argument(error);
  try {
    const dslvm::FunctionEntry *entry = r->value.function_by_signature(signature);
    if (!entry) { r->code="LOAD_UNBOUND"; r->message="unknown function signature"; if(error)*error={r->code.c_str(),r->message.c_str(),0,0,0,0}; return 1; }
    dslvm::Args args; args.reserve(count);
    for (size_t i=0; i<count; ++i) args.push_back(to_cpp(values[i]));
    *result = owned_value(r, entry->fn(args));
    return 0;
  } catch (const dslvm::Error &e) { return fill(error, r, e); }
  catch (const std::exception &e) { r->code="RUN_HOST"; r->message=e.what(); if(error)*error={r->code.c_str(),r->message.c_str(),0,0,0,0}; return 1; }
  catch (...) { return unknown_error(error); }
}
int sample_registry_call_method(sample_registry *r, const char *signature,
                                void *self, const sample_value *values, size_t count,
                                sample_value *result, sample_error *error) {
  clear_error(error);
  if (!r || !signature || (count && !values) || !result)
    return bad_argument(error);
  try {
    const dslvm::MethodEntry *entry = r->value.method_by_signature(signature);
    if (!entry) { r->code="LOAD_UNBOUND"; r->message="unknown method signature"; if(error)*error={r->code.c_str(),r->message.c_str(),0,0,0,0}; return 1; }
    dslvm::Args args; args.reserve(count);
    for (size_t i=0; i<count; ++i) args.push_back(to_cpp(values[i]));
    *result = owned_value(r, entry->fn(self, args));
    return 0;
  } catch (const dslvm::Error &e) { return fill(error, r, e); }
  catch (const std::exception &e) { r->code="RUN_HOST"; r->message=e.what(); if(error)*error={r->code.c_str(),r->message.c_str(),0,0,0,0}; return 1; }
  catch (...) { return unknown_error(error); }
}

sample_context *sample_context_create(void) {
  try {
    return new sample_context;
  } catch (...) {
    return nullptr;
  }
}
void sample_context_destroy(sample_context *c) { delete c; }
int sample_context_set(sample_context *c, const char *n, void *o,
                       sample_error *e) {
  clear_error(e);
  if (!c || !n)
    return bad_argument(e);
  try {
    c->value.set(n, o);
    return 0;
  } catch (const std::exception &x) {
    if (e) {
      c->code = "RUN_HOST";
      c->message = x.what();
      *e = {c->code.c_str(), c->message.c_str(), 0, 0, 0, 0};
    }
    return 1;
  }
}
int sample_context_set_file(sample_context *c, const char *f, sample_error *e) {
  clear_error(e);
  if (!c)
    return bad_argument(e);
  try {
    c->file = f ? f : "";
    c->value.set_file(c->file);
    return 0;
  } catch (const std::exception &x) {
    if (e) {
      c->code = "RUN_HOST";
      c->message = x.what();
      *e = {c->code.c_str(), c->message.c_str(), 0, 0, 0, 0};
    }
    return 1;
  }
}
int sample_context_set_dry(sample_context *c, int d, sample_error *e) {
  clear_error(e);
  if (!c)
    return bad_argument(e);
  c->value.set_dry(d != 0);
  return 0;
}
int sample_context_set_step_budget(sample_context *c, uint64_t b,
                                   sample_error *e) {
  clear_error(e);
  if (!c)
    return bad_argument(e);
  c->value.set_step_budget(b);
  return 0;
}

int sample_program_load(const void *b, size_t bl, const void *s, size_t sl,
                        const sample_registry *r, sample_program **out,
                        sample_error *e) {
  clear_error(e);
  if (out)
    *out = nullptr;
  if (!b || !s || !r || !out)
    return bad_argument(e);
  try {
    auto p = new sample_program{
        dslvm::Program::load(std::string((const char *)b, bl),
                             std::string((const char *)s, sl), r->value),
        r,
        {},
        {}};
    *out = p;
    return 0;
  } catch (const dslvm::Error &x) {
    return fill_registry(r, e, x);
  } catch (const std::exception &x) {
    if (e) {
      sample_registry *m = const_cast<sample_registry *>(r);
      m->code = "RUN_HOST";
      m->message = x.what();
      *e = {m->code.c_str(), m->message.c_str(), 0, 0, 0, 0};
    }
    return 1;
  } catch (...) {
    return unknown_error(e);
  }
}
int sample_program_run(const sample_program *p, sample_context *c,
                       sample_error *e) {
  clear_error(e);
  if (!p || !c || !p->registry)
    return bad_argument(e);
  try {
    p->value.run(c->value, p->registry->value);
    return 0;
  } catch (const dslvm::Error &x) {
    if (e) {
      p->code = x.code;
      p->message = x.what();
      *e = {p->code.c_str(), p->message.c_str(), x.line,
            x.column,        x.has_location,     x.has_column};
    }
    return 1;
  } catch (const std::exception &x) {
    if (e) {
      p->code = "RUN_HOST";
      p->message = x.what();
      *e = {p->code.c_str(), p->message.c_str(), 0, 0, 0, 0};
    }
    return 1;
  } catch (...) {
    return unknown_error(e);
  }
}
void sample_program_destroy(sample_program *p) { delete p; }
int sample_report_ok_json(const char *j) {
  try {
    return dslvm::report_ok(j ? j : "null");
  } catch (...) {
    return 3;
  }
}
int sample_report_error(const char *phase, const sample_error *e) {
  try {
    dslvm::Error x(e && e->code ? e->code : "RUN_HOST",
                   e && e->message ? e->message : "error");
    if (e && e->has_location) {
      x.has_location = true;
      x.has_column = e->has_column;
      x.line = e->line;
      x.column = e->column;
    }
    return dslvm::report_error(phase ? phase : "run", x);
  } catch (...) {
    return 3;
  }
}
