#ifndef MS_OPTIONS_H
#define MS_OPTIONS_H

#include <boost/optional/optional.hpp>

#include <string>

#include "../../structures/types.h"

#include "../aoluarunner/options.h"

struct MSOptions {
  BaselineIOMode ioMode;
  std::string dataColumnName;
  bool subtractModel, combineSPWs;
  boost::optional<size_t> intervalStart, intervalEnd;

  BaselineIntegration baselineIntegration;
};

#endif
