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
};
} // namespace sample
