#ifndef MSIMAGESET_H
#define MSIMAGESET_H

#include <set>
#include <string>
#include <stdexcept>

#include "../../structures/antennainfo.h"
#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"
#include "../../structures/msmetadata.h"

#include "../../msio/baselinereader.h"

#include "indexableset.h"

#include "../../util/logger.h"

namespace rfiStrategy {

class MSImageSet final : public IndexableSet {
 public:
  MSImageSet(const std::string &location, BaselineIOMode ioMode)
      : _msFile(location),
        _metaData(location),
        _reader(),
        _dataColumnName("DATA"),
        _intervalStart(),
        _intervalEnd(),
        _subtractModel(false),
        _bandCount(0),
        _fieldCount(0),
        _sequencesPerBaselineCount(0),
        _readFlags(true),
        _readUVW(false),
        _ioMode(ioMode) {}

  MSImageSet(const MSImageSet &) = default;

  ~MSImageSet() {}

  size_t StartTimeIndex(const ImageSetIndex &index) const { return 0; }

  size_t EndTimeIndex(const ImageSetIndex &index) const;

  std::unique_ptr<ImageSet> Clone() override {
    return std::unique_ptr<MSImageSet>(new MSImageSet(*this));
  }

  size_t Size() const override { return _sequences.size(); }
  std::string Description(const ImageSetIndex &index) const override;
  std::string Name() const override { return _metaData.Path(); }
  std::vector<std::string> Files() const override {
    return std::vector<std::string>{_metaData.Path()};
  }

  void AddReadRequest(const ImageSetIndex &index) override;
  void PerformReadRequests(class ProgressListener &progress) override;
  std::unique_ptr<BaselineData> GetNextRequested() override;

  void AddWriteFlagsTask(const ImageSetIndex &index,
                         std::vector<Mask2DCPtr> &flags) override;
  void PerformWriteFlagsTask() override;

  void Initialize() override;

  void PerformWriteDataTask(const ImageSetIndex &index,
                            std::vector<Image2DCPtr> realImages,
                            std::vector<Image2DCPtr> imaginaryImages) override {
    _reader->PerformDataWriteTask(realImages, imaginaryImages,
                                  GetAntenna1(index), GetAntenna2(index),
                                  GetBand(index), GetSequenceId(index));
  }

  BaselineReaderPtr Reader() const override { return _reader; }

  MSMetaData &MetaData() { return _metaData; }

  size_t GetAntenna1(const ImageSetIndex &index) const override {
    return _sequences[index.Value()].antenna1;
  }
  size_t GetAntenna2(const ImageSetIndex &index) const override {
    return _sequences[index.Value()].antenna2;
  }
  size_t GetBand(const ImageSetIndex &index) const override {
    return _sequences[index.Value()].spw;
  }
  size_t GetField(const ImageSetIndex &index) const override {
    return _sequences[index.Value()].fieldId;
  }
  size_t GetSequenceId(const ImageSetIndex &index) const override {
    return _sequences[index.Value()].sequenceId;
  }

  boost::optional<ImageSetIndex> Index(size_t antenna1, size_t antenna2,
                                       size_t bandIndex,
                                       size_t sequenceId) const override {
    size_t value = findBaselineIndex(antenna1, antenna2, bandIndex, sequenceId);
    if (value == not_found)
      return boost::optional<ImageSetIndex>();
    else
      return ImageSetIndex(Size(), value);
  }

  const std::string &DataColumnName() const { return _dataColumnName; }
  void SetDataColumnName(const std::string &name) {
    if (_reader != 0)
      throw std::runtime_error(
          "Trying to set data column after creating the reader!");
    _dataColumnName = name;
  }

  bool SubtractModel() const { return _subtractModel; }
  void SetSubtractModel(bool subtractModel) {
    if (_reader != 0)
      throw std::runtime_error(
          "Trying to set model subtraction after creating the reader!");
    _subtractModel = subtractModel;
  }

  size_t BandCount() const override { return _bandCount; }
  class ::AntennaInfo GetAntennaInfo(unsigned antennaIndex) const override {
    return _metaData.GetAntennaInfo(antennaIndex);
  }
  class ::BandInfo GetBandInfo(unsigned bandIndex) const override {
    return _metaData.GetBandInfo(bandIndex);
  }
  size_t SequenceCount() const override { return _sequencesPerBaselineCount; }

  size_t AntennaCount() const override { return _metaData.AntennaCount(); }
  class ::FieldInfo GetFieldInfo(unsigned fieldIndex) const override {
    return _metaData.GetFieldInfo(fieldIndex);
  }
  std::vector<double> ObservationTimesVector(const ImageSetIndex &index);
  size_t FieldCount() const { return _fieldCount; }
  void SetReadFlags(bool readFlags) { _readFlags = readFlags; }
  void SetReadUVW(bool readUVW) { _readUVW = readUVW; }
  const std::vector<MSMetaData::Sequence> &Sequences() const {
    return _sequences;
  }

  void SetInterval(boost::optional<size_t> start, boost::optional<size_t> end) {
    _intervalStart = start;
    _intervalEnd = end;
    if (start) _metaData.SetIntervalStart(start.get());
    if (end) _metaData.SetIntervalEnd(end.get());
  }

 private:
  MSImageSet(const std::string &location, BaselineReaderPtr reader)
      : _msFile(location),
        _metaData(location),
        _reader(reader),
        _dataColumnName("DATA"),
        _subtractModel(false),
        _readFlags(true),
        _readUVW(false),
        _ioMode(AutoReadMode) {}
  void initReader();
  const static size_t not_found = std::numeric_limits<size_t>::max();
  size_t findBaselineIndex(size_t antenna1, size_t antenna2, size_t band,
                           size_t sequenceId) const;

  TimeFrequencyMetaDataCPtr createMetaData(const ImageSetIndex &index,
                                           std::vector<UVW> &uvw);

  const std::string _msFile;
  MSMetaData _metaData;
  BaselineReaderPtr _reader;
  std::string _dataColumnName;
  boost::optional<size_t> _intervalStart, _intervalEnd;
  bool _subtractModel;
  std::vector<MSMetaData::Sequence> _sequences;
  size_t _bandCount, _fieldCount, _sequencesPerBaselineCount;
  bool _readFlags, _readUVW;
  BaselineIOMode _ioMode;
  std::vector<BaselineData> _baselineData;
};

}  // namespace rfiStrategy

#endif
