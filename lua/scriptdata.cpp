#include "scriptdata.h"

#include "../algorithms/bandpassfile.h"

#include "../quality/statisticscollection.h"

ScriptData::ScriptData()
    : _progressListener(nullptr),
      _bandpassFile(),
      _bandpassMutex(),
      _canVisualize(false),
      _visualizationData(),
      _statistics() {}

ScriptData::~ScriptData() {}

void ScriptData::Combine(ScriptData&& other) {
  if (!_bandpassFile) _bandpassFile = std::move(other._bandpassFile);
  if (_canVisualize) {
    _visualizationData.insert(_visualizationData.end(),
                              other._visualizationData.begin(),
                              other._visualizationData.end());
    other._visualizationData.clear();
  }
  if (other._statistics) {
    if (_statistics)
      _statistics->Add(*other._statistics);
    else
      _statistics = std::move(other._statistics);
  }
}
