#ifndef SAMPLE_DOMAIN_H
#define SAMPLE_DOMAIN_H
#include <stddef.h>
#include <stdint.h>
typedef struct SampleMoney {
  int64_t cents;
} SampleMoney;
typedef struct SampleItems {
  SampleMoney **values;
  size_t count, capacity;
} SampleItems;
typedef struct SampleShop {
  SampleItems items;
} SampleShop;
typedef struct SampleLedger {
  int64_t balance, entries;
  char *note;
  size_t note_length;
} SampleLedger;
typedef struct SampleDomain {
  SampleShop shop;
  SampleLedger ledger;
  SampleMoney **arena;
  size_t arena_count, arena_capacity;
} SampleDomain;
#ifdef __cplusplus
extern "C" {
#endif
void sample_domain_init(SampleDomain *domain);
void sample_domain_destroy(SampleDomain *domain);
SampleMoney *sample_money_new(SampleDomain *, int64_t cents);
SampleMoney *sample_money_get(void *pointer);
int64_t sample_wrap_add(int64_t a, int64_t b);
int64_t sample_wrap_sub(int64_t a, int64_t b);
int64_t sample_wrap_mul(int64_t a, int64_t b);
int64_t sample_wrap_neg(int64_t a);
int sample_units_to_cents(double units, int64_t *cents);
int sample_shop_total(const SampleShop *, int64_t *total);
SampleMoney *sample_items_at(const SampleItems *, int64_t index);
int sample_ledger_record(SampleLedger *, const SampleMoney *money);
int sample_ledger_note(SampleLedger *, const char *data, size_t length);
#ifdef __cplusplus
}
#endif
#endif
