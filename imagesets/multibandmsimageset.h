#ifndef MULTIBANDMSIMAGESET_H
#define MULTIBANDMSIMAGESET_H

#include "../msio/baselinereader.h"

#include "indexableset.h"

#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace imagesets {

struct MetaData;

/**
 * The multiband ms image set combines multiple single band measurement
 * sets by spectrally concatenating the data.
 *
 * The \a names contains the measurement sets. They should be part of the
 * same observation, where every file contains one subband of the
 * observation. The files are listed in consecutive order and all
 * subbands should be present.
 */
class MultiBandMsImageSet final : public IndexableSet {
 public:
  MultiBandMsImageSet(const std::vector<std::string>& names,
                      BaselineIOMode io_mode,
                      std::optional<size_t> start_time_step,
                      std::optional<size_t> end_time_step, size_t n_threads);

  MultiBandMsImageSet(const MultiBandMsImageSet&) = delete;

  ~MultiBandMsImageSet() = default;

  MultiBandMsImageSet& operator=(const MultiBandMsImageSet&) = delete;

  void WriteToMs(size_t n_threads);

  std::unique_ptr<ImageSet> Clone() override {
    throw std::runtime_error("Not available");
  }

  const std::vector<MSMetaData::Sequence>& Sequences() const {
    return sequences_;
  }

  size_t Size() const override { return sequences_.size() * BandCount(); }

  std::string Description(const ImageSetIndex& index) const override;

  std::string Name() const override {
    return "Spectrally concatenated set (" + ms_names_.front() + " ...)";
  }
  std::vector<std::string> Files() const override { return ms_names_; }

  void AddReadRequest(const ImageSetIndex& index) override;

  void PerformReadRequests(class ProgressListener& progress) override;
  std::unique_ptr<BaselineData> GetNextRequested() override;

  void AddWriteFlagsTask(const ImageSetIndex& index,
                         std::vector<Mask2DCPtr>& flags) override;

  void PerformWriteFlagsTask() override;

  void Initialize() override { /* Do nothing.*/
  }

  void PerformWriteDataTask(const ImageSetIndex& index,
                            std::vector<Image2DCPtr> realImages,
                            std::vector<Image2DCPtr> imaginaryImages) override {
    throw std::runtime_error("Not implemented");
  }

  BaselineReaderPtr Reader() const override {
    throw std::runtime_error("Not available");
  }

  std::string TelescopeName() override {
    casacore::MeasurementSet ms = readers_[0]->OpenMS();
    return MSMetaData::GetTelescopeName(ms);
  }

  size_t GetAntenna1(const ImageSetIndex& index) const override {
    return GetSequence(index).antenna1;
  }
  size_t GetAntenna2(const ImageSetIndex& index) const override {
    return GetSequence(index).antenna2;
  }
  size_t GetBand(const ImageSetIndex&) const override { return 0; }
  size_t GetField(const ImageSetIndex& index) const override {
    return GetSequence(index).fieldId;
  }
  size_t GetSequenceId(const ImageSetIndex& index) const override {
    return GetSequence(index).sequenceId;
  }

  std::optional<ImageSetIndex> Index(size_t antenna_1, size_t antenna_2,
                                     size_t band,
                                     size_t sequence_id) const override;

  size_t BandCount() const override { return 1; }
  AntennaInfo GetAntennaInfo(unsigned antennaIndex) const override {
    return antennae_[antennaIndex];
  }
  BandInfo GetBandInfo(unsigned) const override { return band_; }

  size_t SequenceCount() const override {
    return observation_times_per_sequence_.size();
  }

  size_t AntennaCount() const override { return antennae_.size(); }
  FieldInfo GetFieldInfo(unsigned fieldIndex) const override {
    return fields_[fieldIndex];
  }

 private:
  size_t GetSequenceIndex(const ImageSetIndex& index) const {
    return index.Value();
  }
  MSMetaData::Sequence GetSequence(const ImageSetIndex& index) const {
    return sequences_[GetSequenceIndex(index)];
  }
  size_t EndTimeIndex(size_t sequence_id) const {
    return observation_times_per_sequence_[sequence_id].size();
  }
  size_t FindBaselineIndex(size_t antenna_1, size_t antenna_2, size_t band,
                           size_t sequence_id) const;

  void ReadData(size_t n_threads);
  void ProcessMetaData();
  std::unique_ptr<BaselineData> CombineData(const ImageSetIndex& index);

  const static size_t kNotFound;
  std::vector<std::string> ms_names_;

  // TODO Other readers use a vector and call pop_front, if the vector contains
  // a lot of elements this is inefficient.
  std::deque<std::unique_ptr<BaselineData>> data_;
  std::vector<ImageSetIndex> read_requests_;

  // All measurement sets read contain the same metadata this a cached copy.
  std::vector<AntennaInfo> antennae_;
  std::vector<FieldInfo> fields_;
  std::vector<MSMetaData::Sequence> sequences_;
  std::vector<std::set<double>> observation_times_per_sequence_;

  // Contains the information of all concatenated channels
  BandInfo band_;
  // Contains the number of channels per band.
  // This is needed to split the total image per band for the writing step.
  std::vector<size_t> channels_per_band_;

  std::vector<std::unique_ptr<BaselineReader>> readers_;
};

}  // namespace imagesets

#endif  // MULTIBANDMSIMAGESET_H
