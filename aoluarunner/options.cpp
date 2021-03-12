#include "options.h"

#include "../structures/system.h"

size_t Options::CalculateThreadCount() const {
  if (threadCount)
    return threadCount;
  else
    return System::ProcessorCount();
}
