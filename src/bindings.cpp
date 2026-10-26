#include "bindings.hpp"
#include "sample_schema.hpp"
#include "testing.hpp"
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
namespace sample {
static dslvm::Value money(Host *h, int64_t c) {
  auto *p = sample_money_new(&h->domain, c);
  if (!p)
    throw dslvm::Error("RUN_HOST", "money: allocation failed");
  return dslvm::Value::object("Money", p);
}
static SampleMoney &money_ref(void *p) {
  if (!p)
    throw dslvm::Error("RUN_HOST", "money: null handle");
  return *sample_money_get(p);
}
static dslvm::Value units(Host *h, double x) {
  int64_t c = 0;
  if (sample_units_to_cents(x, &c))
    throw dslvm::Error("RUN_HOST", "money: amount out of range");
  return money(h, c);
}
static dslvm::Value fn(Host *h, const std::string &n, dslvm::Args &a) {
  using dslvm::Value;
  if (n == "money")
    return a[0].kind == Value::INT ? money(h, a[0].i) : units(h, a[0].f);
  if (n == "fee")
    return Value::integer(sample_wrap_add(a[0].i, 5));
  if (n == "sample_tmp") {
    const char *p = getenv("SAMPLE_TMP");
    if (!p || !*p)
      throw dslvm::Error("RUN_HOST", "SAMPLE_TMP is required");
    return Value::string(p);
  }
  if (n == "host_int")
    return Value::integer(a[0].i);
  if (n == "host_float")
    return Value::real(a[0].f);
  if (n == "host_bool")
    return Value::boolean(a[0].b);
  if (n == "host_bytes")
    return Value::string(a[0].s);
  if (n == "host_hex")
    return Value::string(dslvm::hex(a[0].s));
  if (n == "host_float_case") {
    if (a[0].s == "-0")
      return Value::real(-0.0);
    if (a[0].s == "nan")
      return Value::real(NAN);
    if (a[0].s == "inf")
      return Value::real(INFINITY);
    if (a[0].s == "-inf")
      return Value::real(-INFINITY);
    if (a[0].s == "large")
      return Value::real(1e21);
    if (a[0].s == "subnormal")
      return Value::real(std::numeric_limits<double>::denorm_min());
    throw dslvm::Error("RUN_HOST", "host_float_case: unknown case");
  }
  if (n == "host_echo_money")
    return a[0];
  if (n == "host_same")
    return Value::boolean(a[0].obj == a[1].obj);
  if (n == "host_null")
    return Value::object("Money", nullptr);
  if (n == "host_is_null")
    return Value::boolean(!a[0].obj);
  if (n == "host_wrong")
    return Value::object("Items", nullptr);
  if (n == "host_void")
    return Value::none();
  if (n == "host_sum") {
    uint64_t z = 0;
    for (auto &v : a)
      z += (uint64_t)v.i;
    return Value::integer(sample_wrap_add((int64_t)z, 0));
  }
  if (n == "host_str_list") {
    auto *in = static_cast<dslvm::stdlib::StrList *>(a[0].obj);
    auto values = in->items;
    values.push_back("host");
    return Value::object("StrList", h->registry.std_collections().make_str_list(
                                        std::move(values)));
  }
  if (n == "host_int_list") {
    auto *in = static_cast<dslvm::stdlib::IntList *>(a[0].obj);
    auto values = in->items;
    values.push_back(9);
    return Value::object("IntList", h->registry.std_collections().make_int_list(
                                        std::move(values)));
  }
  if (n == "host_float_list") {
    auto *in = static_cast<dslvm::stdlib::FloatList *>(a[0].obj);
    auto values = in->items;
    values.push_back(.5);
    return Value::object(
        "FloatList",
        h->registry.std_collections().make_float_list(std::move(values)));
  }
  if (n == "host_str_map") {
    auto *in = static_cast<dslvm::stdlib::StrMap *>(a[0].obj);
    auto *out = h->registry.std_collections().make_str_map();
    for (const auto &item : in->entries())
      out->set(item.first, item.second);
    out->set("host", "yes");
    return Value::object("StrMap", out);
  }
  if (n.rfind("host_long_", 0) == 0)
    return Value::integer(a[0].i);
  throw dslvm::Error("RUN_HOST", "unknown sample function");
}
static dslvm::Value method(Host *h, const std::string &owner,
                           const std::string &n, void *self, dslvm::Args &a) {
  using dslvm::Value;
  if (owner == "Money") {
    auto &m = money_ref(self);
    if (n == "amount")
      return Value::integer(m.cents);
    if (n == "currency")
      return Value::string("USD");
    if (n == "set") {
      m.cents = a[0].i;
      return Value::object("Money", self);
    }
    if (n == "plus") {
      return money(h, sample_wrap_add(m.cents, money_ref(a[0].obj).cents));
    }
    if (n == "minus") {
      return money(h, sample_wrap_sub(m.cents, money_ref(a[0].obj).cents));
    }
    if (n == "times")
      return money(h, sample_wrap_mul(m.cents, a[0].i));
    if (n == "neg")
      return money(h, sample_wrap_neg(m.cents));
  }
  if (owner == "Items") {
    auto &i = *(SampleItems *)self;
    if (n == "len")
      return Value::integer((int64_t)i.count);
    if (n == "at") {
      if (a[0].i < 0 || (uint64_t)a[0].i >= i.count)
        throw dslvm::Error("RUN_HOST", "items: index out of range");
      return Value::object("Money", i.values[a[0].i]);
    }
  }
  if (owner == "Shop") {
    auto &s = *(SampleShop *)self;
    if (n == "items")
      return Value::object("Items", &s.items);
    if (n == "total") {
      int64_t z = 0;
      if (sample_shop_total(&s, &z))
        throw dslvm::Error("RUN_HOST", "shop: invalid item handle");
      return money(h, z);
    }
    if (n == "price")
      return a[0].kind == Value::INT ? money(h, a[0].i) : units(h, a[0].f);
    if (n == "currency")
      return Value::string("USD");
    if (n == "label")
      return Value::string("sample shop");
  }
  if (owner == "Ledger") {
    auto &l = *(SampleLedger *)self;
    if (n == "record") {
      if (sample_ledger_record(&l, &money_ref(a[0].obj)))
        throw dslvm::Error("RUN_HOST", "ledger: invalid money handle");
      return Value::object("Ledger", self);
    }
    if (n == "balance")
      return money(h, l.balance);
    if (n == "count")
      return Value::integer(l.entries);
    if (n == "note") {
      if (a.size()) {
        if (sample_ledger_note(&l, a[0].s.data(), a[0].s.size()))
          throw dslvm::Error("RUN_HOST", "ledger: note allocation failed");
        return Value::none();
      }
      return Value::string(l.note ? std::string(l.note, l.note_length)
                                  : std::string());
    }
    if (n == "reject")
      throw dslvm::Error("RUN_HOST", a[0].s);
  }
  throw dslvm::Error("RUN_HOST", "unknown sample method");
}
Host::Host() {
  using dslvm::Args;
  using dslvm::Value;
  registry.context("shop", "Shop").context("ledger", "Ledger");
  dslvm::stdlib::register_testing(registry);
  sample_domain_init(&domain);
  auto addf = [this](std::string n, std::vector<std::string> p, const char *r) {
    registry.function(n, p, r, [this, n](Args &a) { return fn(this, n, a); });
  };
  auto addm = [this](const char *t, const char *n, std::vector<std::string> p,
                     const char *r) {
    registry.method(t, n, p, r, [this, t, n](void *s, Args &a) {
      return method(this, t, n, s, a);
    });
  };
  addf("money", {"int"}, "Money");
  addf("money", {"float"}, "Money");
  addf("fee", {"int"}, "int");
  addf("sample_tmp", {}, "string");
  for (auto x : std::vector<std::pair<
           const char *, std::pair<std::vector<std::string>, const char *>>>{
           {"host_int", {{"int"}, "int"}},
           {"host_float", {{"float"}, "float"}},
           {"host_bool", {{"bool"}, "bool"}},
           {"host_bytes", {{"string"}, "string"}},
           {"host_hex", {{"string"}, "string"}},
           {"host_float_case", {{"string"}, "float"}},
           {"host_echo_money", {{"Money"}, "Money"}},
           {"host_same", {{"Money", "Money"}, "bool"}},
           {"host_null", {{}, "Money"}},
           {"host_is_null", {{"Money"}, "bool"}},
           {"host_wrong", {{}, "Money"}},
           {"host_void", {{}, "void"}}})
    addf(x.first, x.second.first, x.second.second);
  addf("host_sum", std::vector<std::string>(255, "int"), "int");
  addf("host_long_" + std::string(128, 'x'), {"int"}, "int");
  addf("host_str_list", {"StrList"}, "StrList");
  addf("host_int_list", {"IntList"}, "IntList");
  addf("host_float_list", {"FloatList"}, "FloatList");
  addf("host_str_map", {"StrMap"}, "StrMap");
  addm("Money", "amount", {}, "int");
  addm("Money", "currency", {}, "Currency");
  addm("Money", "set", {"int"}, "Money");
  addm("Money", "plus", {"Money"}, "Money");
  addm("Money", "minus", {"Money"}, "Money");
  addm("Money", "times", {"int"}, "Money");
  addm("Money", "neg", {}, "Money");
  addm("Items", "len", {}, "int");
  addm("Items", "at", {"int"}, "Money");
  addm("Shop", "items", {}, "Items");
  addm("Shop", "total", {}, "Money");
  addm("Shop", "price", {"int"}, "Money");
  addm("Shop", "price", {"float"}, "Money");
  addm("Shop", "currency", {}, "Currency");
  addm("Shop", "label", {}, "string");
  addm("Ledger", "record", {"Money"}, "Ledger");
  addm("Ledger", "balance", {}, "Money");
  addm("Ledger", "count", {}, "int");
  addm("Ledger", "note", {"string"}, "void");
  addm("Ledger", "note", {}, "string");
  addm("Ledger", "reject", {"string"}, "void");
}
Host::~Host() { sample_domain_destroy(&domain); }
dslvm::Context Host::context() {
  dslvm::Context c;
  c.set("shop", &domain.shop);
  c.set("ledger", &domain.ledger);
  return c;
}
} // namespace sample
