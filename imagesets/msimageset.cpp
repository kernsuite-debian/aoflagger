#include <iostream>
#include <sstream>
#include <stdexcept>

#include "../algorithms/timefrequencystatistics.h"

#include "msimageset.h"

#include "../msio/directbaselinereader.h"
#include "../msio/indirectbaselinereader.h"
#include "../msio/memorybaselinereader.h"

#include "../util/logger.h"

namespace imagesets {

void MSImageSet::Initialize() {
  Logger::Debug << "Initializing image set...\n";
  Logger::Debug << "Antennas: " << _metaData.AntennaCount() << '\n';
  _sequences = _metaData.GetSequences();
  Logger::Debug << "Unique sequences: " << _sequences.size() << '\n';
  if (_sequences.empty())
    throw std::runtime_error(
        "Trying to open a measurement set with no sequences");
  initReader();
  _bandCount = _metaData.BandCount();
  _fieldCount = _metaData.FieldCount();
  _sequencesPerBaselineCount = _metaData.SequenceCount();
  Logger::Debug << "Bands: " << _bandCount << '\n';
}

void MSImageSet::initReader() {
  if (_reader == nullptr) {
    switch (_ioMode) {
      case IndirectReadMode: {
        IndirectBaselineReader* indirectReader =
            new IndirectBaselineReader(_msFile);
        indirectReader->SetReadUVW(_readUVW);
        _reader = BaselineReaderPtr(indirectReader);
      } break;
      case DirectReadMode:
        _reader = BaselineReaderPtr(new DirectBaselineReader(_msFile));
        break;
      case MemoryReadMode:
        _reader = BaselineReaderPtr(new MemoryBaselineReader(_msFile));
        break;
      case AutoReadMode:
        if (MemoryBaselineReader::IsEnoughMemoryAvailable(_msFile))
          _reader = BaselineReaderPtr(new MemoryBaselineReader(_msFile));
        else
          _reader = BaselineReaderPtr(new DirectBaselineReader(_msFile));
        break;
    }
  }
  _reader->SetDataColumnName(_dataColumnName);
  _reader->SetInterval(_intervalStart, _intervalEnd);
  _reader->SetReadFlags(_readFlags);
  _reader->SetReadData(true);
}

size_t MSImageSet::EndTimeIndex(const ImageSetIndex& index) const {
  return _reader->MetaData()
      .GetObservationTimesSet(GetSequenceId(index))
      .size();
}

std::vector<double> MSImageSet::ObservationTimesVector(
    const ImageSetIndex& index) {
  // StartIndex(msIndex), EndIndex(msIndex)
  unsigned sequenceId = _sequences[index.Value()].sequenceId;
  const std::set<double>& obsTimesSet =
      _reader->MetaData().GetObservationTimesSet(sequenceId);
  std::vector<double> obs(obsTimesSet.begin(), obsTimesSet.end());
  return obs;
}

TimeFrequencyMetaDataCPtr MSImageSet::createMetaData(const ImageSetIndex& index,
                                                     std::vector<UVW>& uvw) {
  TimeFrequencyMetaData* metaData = new TimeFrequencyMetaData();
  metaData->SetAntenna1(_metaData.GetAntennaInfo(GetAntenna1(index)));
  metaData->SetAntenna2(_metaData.GetAntennaInfo(GetAntenna2(index)));
  metaData->SetBand(_metaData.GetBandInfo(GetBand(index)));
  metaData->SetField(_metaData.GetFieldInfo(GetField(index)));
  metaData->SetObservationTimes(ObservationTimesVector(index));
  if (_reader != nullptr) {
    metaData->SetUVW(uvw);
  }
  return TimeFrequencyMetaDataCPtr(metaData);
}

std::string MSImageSet::Description(const ImageSetIndex& index) const {
  std::stringstream sstream;
  const MSMetaData::Sequence& sequence = _sequences[index.Value()];
  size_t antenna1 = sequence.antenna1, antenna2 = sequence.antenna2,
         band = sequence.spw, sequenceId = sequence.sequenceId;
  AntennaInfo info1 = GetAntennaInfo(antenna1);
  AntennaInfo info2 = GetAntennaInfo(antenna2);
  sstream << info1.station << ' ' << info1.name << " x " << info2.station << ' '
          << info2.name;
  if (BandCount() > 1) {
    BandInfo bandInfo = GetBandInfo(band);
    double bandStart =
        round(bandInfo.channels.front().frequencyHz / 100000.0) / 10.0;
    double bandEnd =
        round(bandInfo.channels.back().frequencyHz / 100000.0) / 10.0;
    sstream << ", spw " << band << " (" << bandStart << "MHz -" << bandEnd
            << "MHz)";
  }
  if (SequenceCount() > 1) {
    sstream << ", seq " << sequenceId;
  }
  return sstream.str();
}

size_t MSImageSet::findBaselineIndex(size_t antenna1, size_t antenna2,
                                     size_t band, size_t sequenceId) const {
  size_t index = 0;
  for (std::vector<MSMetaData::Sequence>::const_iterator i = _sequences.begin();
       i != _sequences.end(); ++i) {
    bool antennaMatch = (i->antenna1 == antenna1 && i->antenna2 == antenna2) ||
                        (i->antenna1 == antenna2 && i->antenna2 == antenna1);
    if (antennaMatch && i->spw == band && i->sequenceId == sequenceId) {
      return index;
    }
    ++index;
  }
  return not_found;
}

void MSImageSet::AddReadRequest(const ImageSetIndex& index) {
  BaselineData newRequest(index);
  _baselineData.push_back(newRequest);
}

void MSImageSet::PerformReadRequests(class ProgressListener& progress) {
  for (BaselineData& bd : _baselineData) {
    _reader->AddReadRequest(GetAntenna1(bd.Index()), GetAntenna2(bd.Index()),
                            GetBand(bd.Index()), GetSequenceId(bd.Index()),
                            StartTimeIndex(bd.Index()),
                            EndTimeIndex(bd.Index()));
  }

  _reader->PerformReadRequests(progress);

  for (std::vector<BaselineData>::iterator i = _baselineData.begin();
       i != _baselineData.end(); ++i) {
    if (!i->Data().IsEmpty())
      throw std::runtime_error(
          "ReadRequest() called, but a previous read request was not "
          "completely processed by calling GetNextRequested().");
    std::vector<UVW> uvw;
    TimeFrequencyData data = _reader->GetNextResult(uvw);
    i->SetData(data);
    TimeFrequencyMetaDataCPtr metaData = createMetaData(i->Index(), uvw);
    i->SetMetaData(metaData);
  }
}

std::unique_ptr<BaselineData> MSImageSet::GetNextRequested() {
  std::unique_ptr<BaselineData> top(new BaselineData(_baselineData.front()));
  _baselineData.erase(_baselineData.begin());
  if (top->Data().IsEmpty())
    throw std::runtime_error(
        "Calling GetNextRequested(), but requests were not read with "
        "LoadRequests.");
  return top;
}

void MSImageSet::AddWriteFlagsTask(const ImageSetIndex& index,
                                   std::vector<Mask2DCPtr>& flags) {
  size_t seqIndex = index.Value();
  initReader();
  size_t a1 = _sequences[seqIndex].antenna1;
  size_t a2 = _sequences[seqIndex].antenna2;
  size_t b = _sequences[seqIndex].spw;
  size_t s = _sequences[seqIndex].sequenceId;

  std::vector<Mask2DCPtr> allFlags;
  if (flags.size() > _reader->Polarizations().size())
    throw std::runtime_error(
        "Trying to write more polarizations to image set than available");
  else if (flags.size() < _reader->Polarizations().size()) {
    if (flags.size() == 1)
      for (size_t i = 0; i < _reader->Polarizations().size(); ++i)
        allFlags.emplace_back(flags[0]);
    else
      throw std::runtime_error(
          "Incorrect number of polarizations in write action");
  } else
    allFlags = flags;

  _reader->AddWriteTask(allFlags, a1, a2, b, s);
}

void MSImageSet::PerformWriteFlagsTask() {
  _reader->PerformFlagWriteRequests();
}
}  // namespace imagesets
