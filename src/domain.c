#include "domain.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
int64_t sample_wrap_add(int64_t a, int64_t b) {
  uint64_t v = (uint64_t)a + (uint64_t)b;
  return (v & UINT64_C(0x8000000000000000))
             ? INT64_MIN + (int64_t)(v & UINT64_C(0x7fffffffffffffff))
             : (int64_t)v;
}
int64_t sample_wrap_sub(int64_t a, int64_t b) {
  return sample_wrap_add(a, sample_wrap_neg(b));
}
int64_t sample_wrap_mul(int64_t a, int64_t b) {
  uint64_t v = (uint64_t)a * (uint64_t)b;
  return (v & UINT64_C(0x8000000000000000))
             ? INT64_MIN + (int64_t)(v & UINT64_C(0x7fffffffffffffff))
             : (int64_t)v;
}
int64_t sample_wrap_neg(int64_t a) {
  uint64_t v = 0 - (uint64_t)a;
  return (v & UINT64_C(0x8000000000000000))
             ? INT64_MIN + (int64_t)(v & UINT64_C(0x7fffffffffffffff))
             : (int64_t)v;
}
SampleMoney *sample_money_new(SampleDomain *d, int64_t cents) {
  if (d->arena_count == d->arena_capacity) {
    size_t c = d->arena_capacity ? d->arena_capacity * 2 : 16;
    SampleMoney **p = realloc(d->arena, c * sizeof(*p));
    if (!p)
      return NULL;
    d->arena = p;
    d->arena_capacity = c;
  }
  SampleMoney *m = malloc(sizeof(*m));
  if (!m)
    return NULL;
  m->cents = cents;
  d->arena[d->arena_count++] = m;
  return m;
}
SampleMoney *sample_money_get(void *p) { return (SampleMoney *)p; }
int sample_units_to_cents(double units, int64_t *cents) {
  double value = round(units * 100.0);
  if (!isfinite(value) || value < -9223372036854775808.0 ||
      value >= 9223372036854775808.0)
    return 1;
  *cents = (int64_t)value;
  return 0;
}
int sample_shop_total(const SampleShop *s, int64_t *out) {
  if (!s || !out)
    return 1;
  int64_t t = 0;
  for (size_t i = 0; i < s->items.count; i++) {
    if (!s->items.values[i])
      return 1;
    t = sample_wrap_add(t, s->items.values[i]->cents);
  }
  *out = t;
  return 0;
}
SampleMoney *sample_items_at(const SampleItems *i, int64_t index) {
  if (!i || index < 0 || (uint64_t)index >= i->count)
    return NULL;
  return i->values[index];
}
int sample_ledger_record(SampleLedger *l, const SampleMoney *m) {
  if (!l || !m)
    return 1;
  l->balance = sample_wrap_add(l->balance, m->cents);
  l->entries = sample_wrap_add(l->entries, 1);
  return 0;
}
int sample_ledger_note(SampleLedger *l, const char *d, size_t n) {
  char *p = malloc(n + 1);
  if (!p)
    return 1;
  memcpy(p, d, n);
  p[n] = 0;
  free(l->note);
  l->note = p;
  l->note_length = n;
  return 0;
}
void sample_domain_init(SampleDomain *d) {
  memset(d, 0, sizeof(*d));
  for (int64_t i = 1; i <= 3; i++) {
    SampleMoney *m = sample_money_new(d, i * 125);
    if (!m)
      continue;
    SampleMoney **p =
        realloc(d->shop.items.values, (d->shop.items.count + 1) * sizeof(*p));
    if (!p)
      continue;
    d->shop.items.values = p;
    d->shop.items.values[d->shop.items.count++] = m;
  }
}
void sample_domain_destroy(SampleDomain *d) {
  for (size_t i = 0; i < d->arena_count; i++)
    free(d->arena[i]);
  free(d->arena);
  free(d->shop.items.values);
  free(d->ledger.note);
}
