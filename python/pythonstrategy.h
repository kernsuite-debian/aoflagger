#ifndef PYTHON_STRATEGY_H
#define PYTHON_STRATEGY_H

#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"

#include <string>

class PythonStrategy {
 public:
  PythonStrategy();
  ~PythonStrategy();

  void Execute(TimeFrequencyData& tfData, TimeFrequencyMetaDataCPtr metaData,
               class ScriptData& scriptData);

 private:
  std::string _code;
};

#endif
