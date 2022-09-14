
#ifndef JOINED_SPW_SET_H
#define JOINED_SPW_SET_H

#include "imageset.h"
#include "msimageset.h"

#include "../structures/msmetadata.h"

#include <vector>
#include <list>
#include <map>

namespace imagesets {

using Sequence = MSMetaData::Sequence;

class JoinedSPWSet final : public IndexableSet {
 public:
  /**
   * @param msImageSet An already initialized image set of which ownership is
   * transferred to this class.
   */
  explicit JoinedSPWSet(std::unique_ptr<MSImageSet> msImageSet)
      : _msImageSet(std::move(msImageSet)) {
    const std::vector<MSMetaData::Sequence>& sequences =
        _msImageSet->Sequences();
    size_t nBands = _msImageSet->BandCount();
    _nChannels.resize(nBands);
    for (size_t b = 0; b != nBands; ++b)
      _nChannels[b] = _msImageSet->GetBandInfo(b).channels.size();
    std::map<Sequence, std::vector<std::pair<size_t, size_t>>> jsMap;
    for (size_t sequenceIndex = 0; sequenceIndex != sequences.size();
         ++sequenceIndex) {
      const MSMetaData::Sequence& s = sequences[sequenceIndex];
      Sequence js(s.antenna1, s.antenna2, 0, s.sequenceId, s.fieldId);
      // TODO Central frequency instead of spw id is a better idea
      jsMap[js].emplace_back(s.spw, sequenceIndex);
    }
    for (auto& js : jsMap) {
      std::sort(js.second.begin(), js.second.end());
      _joinedSequences.emplace_back(js.first, std::move(js.second));
    }
  }

  JoinedSPWSet(const JoinedSPWSet& source) = delete;
  JoinedSPWSet& operator=(const JoinedSPWSet& source) = delete;

  ~JoinedSPWSet() override{};
  std::unique_ptr<ImageSet> Clone() override {
    std::unique_ptr<JoinedSPWSet> newSet(new JoinedSPWSet());
    newSet->_msImageSet.reset(new MSImageSet(*_msImageSet));
    newSet->_joinedSequences = _joinedSequences;
    newSet->_nChannels = _nChannels;
    return std::move(newSet);
  }

  size_t Size() const override { return _joinedSequences.size(); }
  std::string Description(const ImageSetIndex& index) const override;

  void Initialize() override {}
  std::string Name() const override {
    return _msImageSet->Name() + " (SPWs joined)";
  }
  std::vector<std::string> Files() const override {
    return _msImageSet->Files();
  }

  void AddReadRequest(const ImageSetIndex& index) override {
    const std::vector<std::pair<size_t /*spw*/, size_t /*seq*/>>&
        indexInformation = _joinedSequences[index.Value()].bandAndOriginalSeq;

    for (const std::pair<size_t, size_t>& spwAndSeq : indexInformation) {
      ImageSetIndex msIndex(_msImageSet->Size(), spwAndSeq.second);
      _msImageSet->AddReadRequest(msIndex);
    }
    _requests.push_back(index.Value());
  }

  /**
   * Combines the baseline data of multiple bands in one band.
   *
   * @param data The baseline data of the individual bands.
   * @param totalHeight The sum of the channels in all bands in @a data.
   * @param index The index of the final baseline.
   */
  static std::unique_ptr<BaselineData> CombineBaselineData(
      std::vector<std::unique_ptr<BaselineData>>&& data, size_t totalHeight,
      const ImageSetIndex& index) {
    // Combine the images
    TimeFrequencyData tfData(data[0]->Data());
    size_t width = tfData.ImageWidth();
    for (size_t imgIndex = 0; imgIndex != tfData.ImageCount(); ++imgIndex) {
      size_t chIndex = 0;
      Image2DPtr img = Image2D::CreateUnsetImagePtr(width, totalHeight);
      for (size_t i = 0; i != data.size(); ++i) {
        Image2DCPtr src = data[i]->Data().GetImage(imgIndex);
        for (size_t y = 0; y != src->Height(); ++y) {
          num_t* destPtr = img->ValuePtr(0, y + chIndex);
          const num_t* srcPtr = src->ValuePtr(0, y);
          for (size_t x = 0; x != src->Width(); ++x) destPtr[x] = srcPtr[x];
        }
        chIndex += data[i]->Data().ImageHeight();
      }
      tfData.SetImage(imgIndex, img);
    }

    // Combine the masks
    for (size_t maskIndex = 0; maskIndex != tfData.MaskCount(); ++maskIndex) {
      size_t chIndex = 0;
      Mask2DPtr mask = Mask2D::CreateUnsetMaskPtr(width, totalHeight);
      for (size_t i = 0; i != data.size(); ++i) {
        Mask2DCPtr src = data[i]->Data().GetMask(maskIndex);
        for (size_t y = 0; y != src->Height(); ++y) {
          bool* destPtr = mask->ValuePtr(0, y + chIndex);
          const bool* srcPtr = src->ValuePtr(0, y);
          for (size_t x = 0; x != src->Width(); ++x) destPtr[x] = srcPtr[x];
        }
        chIndex += data[i]->Data().ImageHeight();
      }
      tfData.SetMask(maskIndex, mask);
    }

    // Combine the metadata
    TimeFrequencyMetaDataPtr metaData(
        new TimeFrequencyMetaData(*data[0]->MetaData()));
    BandInfo band = metaData->Band();
    size_t chIndex = band.channels.size();
    band.channels.resize(totalHeight);
    for (size_t i = 1; i != data.size(); ++i) {
      const BandInfo& srcBand = data[i]->MetaData()->Band();
      for (size_t ch = 0; ch != srcBand.channels.size(); ++ch)
        band.channels[ch + chIndex] = srcBand.channels[ch];
      chIndex += srcBand.channels.size();
    }

    metaData->SetBand(band);
    return std::make_unique<BaselineData>(std::move(tfData),
                                          std::move(metaData), index);
  }

  void PerformReadRequests(class ProgressListener& progress) override {
    _msImageSet->PerformReadRequests(progress);
    for (size_t request : _requests) {
      const std::vector<std::pair<size_t /*spw*/, size_t /*seq*/>>&
          indexInformation = _joinedSequences[request].bandAndOriginalSeq;
      std::vector<std::unique_ptr<BaselineData>> data;
      size_t totalHeight = 0;
      for (size_t i = 0; i != indexInformation.size(); ++i) {
        data.emplace_back(_msImageSet->GetNextRequested());
        totalHeight += data.back()->Data().ImageHeight();
      }

      const Sequence seq = _joinedSequences[request].sequence;
      std::optional<ImageSetIndex> index =
          Index(seq.antenna1, seq.antenna2, seq.spw, seq.sequenceId);
      if (!index) throw std::runtime_error("Baseline not found");
      _baselineData.push_back(
          CombineBaselineData(std::move(data), totalHeight, *index));
    }
    _requests.clear();
  }

  std::unique_ptr<BaselineData> GetNextRequested() override {
    std::unique_ptr<BaselineData> result = std::move(_baselineData.front());
    _baselineData.pop_front();
    return result;
  }

  void AddWriteFlagsTask(const ImageSetIndex& index,
                         std::vector<Mask2DCPtr>& flags) override {
    const std::vector<std::pair<size_t /*spw*/, size_t /*seq*/>>&
        indexInformation = _joinedSequences[index.Value()].bandAndOriginalSeq;
    size_t width = flags.front()->Width();
    size_t chIndex = 0;
    for (size_t spw = 0; spw != indexInformation.size(); ++spw) {
      const std::pair<size_t, size_t>& spwAndSeq = indexInformation[spw];
      std::vector<Mask2DCPtr> spwFlags(flags.size());
      for (size_t m = 0; m != flags.size(); ++m) {
        Mask2DPtr spwMask =
            Mask2D::CreateUnsetMaskPtr(width, _nChannels[spwAndSeq.first]);
        for (size_t y = 0; y != _nChannels[spwAndSeq.first]; ++y) {
          const bool* srcPtr = flags[m]->ValuePtr(0, y + chIndex);
          bool* destPtr = spwMask->ValuePtr(0, y);
          for (size_t x = 0; x != flags[m]->Width(); ++x)
            destPtr[x] = srcPtr[x];
        }
        spwFlags[m] = std::move(spwMask);
      }
      chIndex += _nChannels[spw];

      ImageSetIndex msIndex(Size(), spwAndSeq.second);
      _msImageSet->AddWriteFlagsTask(msIndex, spwFlags);
    }
  }
  void PerformWriteFlagsTask() override {
    _msImageSet->PerformWriteFlagsTask();
  }

  BaselineReaderPtr Reader() const override { return _msImageSet->Reader(); }

  MSImageSet& msImageSet() { return *_msImageSet; }

  size_t AntennaCount() const override { return _msImageSet->AntennaCount(); }

  size_t GetAntenna1(const ImageSetIndex& index) const override {
    return _joinedSequences[index.Value()].sequence.antenna1;
  }

  size_t GetAntenna2(const ImageSetIndex& index) const override {
    return _joinedSequences[index.Value()].sequence.antenna2;
  }

  size_t GetBand(const ImageSetIndex& index) const override { return 0; }

  size_t GetField(const ImageSetIndex& index) const override {
    return _joinedSequences[index.Value()].sequence.fieldId;
  }

  size_t GetSequenceId(const ImageSetIndex& index) const override {
    return _joinedSequences[index.Value()].sequence.sequenceId;
  }

  AntennaInfo GetAntennaInfo(unsigned antennaIndex) const override {
    return _msImageSet->GetAntennaInfo(antennaIndex);
  }

  size_t BandCount() const override { return 1; }

  BandInfo GetBandInfo(unsigned bandIndex) const override {
    BandInfo band = _msImageSet->GetBandInfo(0);
    size_t chIndex = band.channels.size();
    for (size_t i = 1; i != _msImageSet->BandCount(); ++i) {
      const BandInfo srcBand = _msImageSet->GetBandInfo(i);
      band.channels.resize(chIndex + srcBand.channels.size());
      for (size_t ch = 0; ch != srcBand.channels.size(); ++ch)
        band.channels[ch + chIndex] = srcBand.channels[ch];
      chIndex += srcBand.channels.size();
    }
    return band;
  }

  size_t SequenceCount() const override { return _msImageSet->SequenceCount(); }

  std::optional<ImageSetIndex> Index(size_t antenna1, size_t antenna2,
                                     size_t bandIndex,
                                     size_t sequenceId) const override {
    for (size_t i = 0; i != _joinedSequences.size(); ++i) {
      const Sequence seq = _joinedSequences[i].sequence;
      bool antennaMatch =
          (seq.antenna1 == antenna1 && seq.antenna2 == antenna2) ||
          (seq.antenna1 == antenna2 && seq.antenna2 == antenna1);
      if (antennaMatch && seq.sequenceId == sequenceId) {
        return ImageSetIndex(Size(), i);
      }
    }
    std::stringstream str;
    str << "Baseline not found: "
        << "antenna1=" << antenna1 << ", "
        << "antenna2=" << antenna2 << ", "
        << "sequenceId=" << sequenceId;
    throw std::runtime_error(str.str());
  }

  FieldInfo GetFieldInfo(unsigned fieldIndex) const override {
    return _msImageSet->GetFieldInfo(fieldIndex);
  }

 private:
  JoinedSPWSet() = default;

  std::unique_ptr<MSImageSet> _msImageSet;
  struct JoinedSequence {
    JoinedSequence(const Sequence& s,
                   std::vector<std::pair<size_t, size_t>>&& bandSeq)
        : sequence(s), bandAndOriginalSeq(std::move(bandSeq)) {}
    Sequence sequence;
    std::vector<std::pair<size_t, size_t>> bandAndOriginalSeq;
  };
  std::vector<JoinedSequence> _joinedSequences;
  std::vector<size_t> _requests;
  std::list<std::unique_ptr<BaselineData>> _baselineData;
  std::vector<size_t> _nChannels;
};

std::string JoinedSPWSet::Description(const ImageSetIndex& index) const {
  const Sequence& sequence = _joinedSequences[index.Value()].sequence;
  size_t antenna1 = sequence.antenna1, antenna2 = sequence.antenna2,
         sequenceId = sequence.sequenceId;
  AntennaInfo info1 = _msImageSet->GetAntennaInfo(antenna1);
  AntennaInfo info2 = _msImageSet->GetAntennaInfo(antenna2);
  std::stringstream sstream;
  sstream << info1.station << ' ' << info1.name << " x " << info2.station << ' '
          << info2.name << " (joined spws)";
  if (_msImageSet->SequenceCount() > 1) {
    sstream << ", seq " << sequenceId;
  }
  return sstream.str();
}
}  // namespace imagesets

#endif
