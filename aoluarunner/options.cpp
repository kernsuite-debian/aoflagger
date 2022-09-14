#include "options.h"

#include <aocommon/system.h>

size_t Options::CalculateThreadCount() const {
  if (threadCount)
    return threadCount;
  else
    return aocommon::system::ProcessorCount();
}
