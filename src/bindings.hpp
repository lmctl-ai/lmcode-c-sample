#pragma once
#include "domain.h"
#include "dslvm.hpp"
namespace sample {
struct Host {
  dslvm::Registry registry;
  SampleDomain domain;
#ifdef H6_TEST_COUNTERS
  size_t sample_tmp_calls = 0;
#endif
  Host();
  ~Host();
  dslvm::Context context();
  dslvm::Value total_handle();
};
} // namespace sample
