#ifndef SAMPLE_SHOP_H
#define SAMPLE_SHOP_H
#include "sample_bridge.h"
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
  sample_registry *registry;
  SampleShop shop;
  SampleLedger ledger;
  SampleMoney **arena;
  size_t arena_count, arena_capacity;
  void **descriptors;
  size_t descriptor_count, descriptor_capacity;
  char *hex_scratch;
  size_t hex_capacity;
} SampleHost;

#ifdef __cplusplus
extern "C" {
#endif
SampleHost *sample_host_create(sample_registry *registry, sample_error *error);
void sample_host_destroy(SampleHost *host);
int sample_host_context(SampleHost *host, sample_context *context,
                        sample_error *error);
#ifdef __cplusplus
}
#endif

#endif
