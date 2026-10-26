#ifndef SAMPLE_SHOP_H
#define SAMPLE_SHOP_H
#include "lmcode_c.h"
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
typedef struct SampleHost {
  lmcode_registry *registry;
  SampleShop shop;
  SampleLedger ledger;
  SampleMoney **arena;
  size_t arena_count, arena_capacity;
} SampleHost;

#ifdef __cplusplus
extern "C" {
#endif
SampleHost *sample_host_create(lmcode_registry *registry, lmcode_error *error);
void sample_host_destroy(SampleHost *host);
int sample_host_context(SampleHost *host, lmcode_context *context,
                        lmcode_error *error);
#ifdef __cplusplus
}
#endif

#endif
