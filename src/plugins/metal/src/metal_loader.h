#pragma once
#include <halley/concurrency/executor.h>
#include <halley/core/api/system_api.h>

namespace Halley {
  class MetalLoader
  {
  public:
    MetalLoader(SystemAPI& system);
    ~MetalLoader();

  private:
    Executor executor;
    std::thread thread;
  };
}
