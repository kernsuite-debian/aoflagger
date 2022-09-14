#ifndef MS_OPTIONS_H
#define MS_OPTIONS_H

#include <optional>
#include <string>

#include "../structures/types.h"

#include "../aoluarunner/options.h"

struct MSOptions {
  BaselineIOMode ioMode;
  std::string dataColumnName;
  bool combineSPWs;
  bool concatenateFrequency;
  std::optional<size_t> intervalStart, intervalEnd;

  BaselineIntegration baselineIntegration;
};

#endif
