#pragma once
#include "domain.h"
#include "dslvm.hpp"
namespace sample {
struct Host {
  dslvm::Registry registry;
  SampleDomain domain;
  Host();
  ~Host();
  dslvm::Context context();
  dslvm::Value total_handle();
};
} // namespace sample
