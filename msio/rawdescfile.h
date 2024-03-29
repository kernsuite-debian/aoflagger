#ifndef RAWDESCFILE_H
#define RAWDESCFILE_H

#include <fstream>
#include <string>
#include <vector>

#include "../util/logger.h"

class RawDescFile {
 public:
  explicit RawDescFile(const std::string& filename) : _filename(filename) {
    readFile();
  }

  size_t GetCount() const { return _sets.size(); }

  std::string GetSet(size_t index) const { return _sets[index]; }

  const std::string& Filename() const { return _filename; }

  unsigned BeamCount() const { return _beamCount; }

  unsigned SubbandCount() const { return _subbandCount; }

  unsigned ChannelsPerSubbandCount() const { return _channelsPerSubbandCount; }

  unsigned TimestepsPerBlockCount() const { return _timestepsPerBlockCount; }

  unsigned BlockHeaderSize() const { return _blockHeaderSize; }

  unsigned BlockFooterSize() const { return _blockFooterSize; }

  unsigned SelectedBeam() const { return _selectedBeam; }

  double TimeResolution() const { return _timeRes; }

  double DisplayedTimeDuration() const { return _displayedTimeDuration; }

  double FrequencyResolution() const { return _freqRes; }

  double FrequencyStart() const { return _freqStart; }

 private:
  const std::string _filename;
  std::vector<std::string> _sets;

  unsigned _beamCount;
  unsigned _subbandCount;
  unsigned _channelsPerSubbandCount;
  unsigned _timestepsPerBlockCount;

  unsigned _blockHeaderSize;
  unsigned _blockFooterSize;

  unsigned _selectedBeam;

  double _timeRes;
  double _displayedTimeDuration;

  double _freqRes;
  double _freqStart;

  void readFile() {
    std::ifstream file(_filename.c_str());
    std::string l;

    std::getline(file, l);
    _beamCount = (unsigned)atol(l.c_str());
    std::getline(file, l);
    _subbandCount = (unsigned)atol(l.c_str());
    std::getline(file, l);
    _channelsPerSubbandCount = (unsigned)atol(l.c_str());
    std::getline(file, l);
    _timestepsPerBlockCount = (unsigned)atol(l.c_str());

    std::getline(file, l);
    _blockHeaderSize = (unsigned)atol(l.c_str());
    std::getline(file, l);
    _blockFooterSize = (unsigned)atol(l.c_str());

    std::getline(file, l);
    _selectedBeam = (unsigned)atol(l.c_str());

    std::getline(file, l);
    _timeRes = atof(l.c_str());
    std::getline(file, l);
    _displayedTimeDuration = atof(l.c_str());
    std::getline(file, l);
    _freqStart = atof(l.c_str());
    std::getline(file, l);
    _freqRes = atof(l.c_str());
    while (file.good()) {
      std::getline(file, l);
      if (l != "") _sets.push_back(l);
    }
  }
};

#endif
