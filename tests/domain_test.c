#include "domain.h"
#include <assert.h>
#include <math.h>
#include <string.h>

int main(void) {
  SampleDomain domain;
  sample_domain_init(&domain);
  assert(sample_wrap_add(INT64_MAX, 1) == INT64_MIN);
  assert(sample_wrap_sub(INT64_MIN, 1) == INT64_MAX);
  assert(sample_wrap_mul(INT64_MAX, 2) == -2);
  assert(sample_wrap_neg(INT64_MIN) == INT64_MIN);
  int64_t cents = 0;
  assert(sample_units_to_cents(1.25, &cents) == 0 && cents == 125);
  assert(sample_units_to_cents(NAN, &cents) != 0);
  assert(sample_units_to_cents(INFINITY, &cents) != 0);
  int64_t total = 0;
  assert(sample_shop_total(&domain.shop, &total) == 0 && total == 750);
  assert(sample_items_at(&domain.shop.items, 0)->cents == 125);
  assert(sample_items_at(&domain.shop.items, 3) == NULL);
  SampleMoney *item = sample_money_new(&domain, 9);
  assert(item && sample_money_get(item) == item);
  assert(sample_ledger_record(&domain.ledger, item) == 0);
  assert(domain.ledger.balance == 9 && domain.ledger.entries == 1);
  assert(sample_ledger_record(&domain.ledger, NULL) != 0);
  const char raw[] = {'a', '\0', 'b'};
  assert(sample_ledger_note(&domain.ledger, raw, 3) == 0);
  assert(domain.ledger.note_length == 3 && !memcmp(domain.ledger.note, raw, 3));
  sample_domain_destroy(&domain);
  return 0;
}
