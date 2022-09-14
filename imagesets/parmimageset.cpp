#include "parmimageset.h"

#include <set>

#include "../msio/parmtable.h"

namespace imagesets {
ParmImageSet::~ParmImageSet() { delete _parmTable; }

void ParmImageSet::Initialize() {
  delete _parmTable;
  _parmTable = new ParmTable(_path);
  const std::set<std::string> antennaSet = _parmTable->GetAntennas();
  for (std::set<std::string>::const_iterator i = antennaSet.begin();
       i != antennaSet.end(); ++i)
    _antennas.push_back(*i);
}

TimeFrequencyData* ParmImageSet::LoadData(const ImageSetIndex& index) {
  const std::string antenna = _antennas[index.Value()];
  return new TimeFrequencyData(_parmTable->Read(antenna));
}
}  // namespace imagesets
