#ifndef FILTERBANKSET_H
#define FILTERBANKSET_H

#include <string>
#include <deque>

#include "imageset.h"

#include "../../util/logger.h"

namespace rfiStrategy {

class FilterBankSet final : public ImageSet {
 public:
  explicit FilterBankSet(const std::string& location);

  ~FilterBankSet() {}

  virtual std::unique_ptr<ImageSet> Clone() override {
    std::unique_ptr<FilterBankSet> set(new FilterBankSet(*this));
    set->_requests.clear();
    return std::move(set);
  }

  size_t Size() const override { return _intervalCount; }

  std::string Name() const override { return _location; }

  std::string Description(const ImageSetIndex& index) const override;

  virtual std::vector<std::string> Files() const override {
    return std::vector<std::string>{_location};
  }

  virtual std::string TelescopeName() override;

  virtual void AddReadRequest(const ImageSetIndex& index) override;

  virtual void PerformReadRequests(class ProgressListener& progress) override;

  virtual std::unique_ptr<BaselineData> GetNextRequested() override;

  virtual void AddWriteFlagsTask(const ImageSetIndex& index,
                                 std::vector<Mask2DCPtr>& flags) override;

  virtual void Initialize() override;

  virtual void PerformWriteDataTask(
      const ImageSetIndex& index, std::vector<Image2DCPtr> realImages,
      std::vector<Image2DCPtr> imaginaryImages) override;

  virtual bool HasCrossCorrelations() const override { return false; }

  double CentreFrequency() const {
    return (_fch1 + (_foff * _channelCount * 0.5)) * 1e6;
  }
  double ChannelWidth() const { return std::fabs(_foff) * 1e6; }
  double TimeResolution() const { return _timeOfSample; }

 private:
  friend class FilterBankSetIndex;
  std::string _location;

  double _timeOfSample, _timeStart, _fch1, _foff;
  size_t _channelCount, _ifCount, _bitCount, _sampleCount;
  size_t _nBeams, _iBeam;
  int _machineId, _telescopeId;
  size_t _intervalCount;
  std::streampos _headerEnd;

  std::deque<BaselineData*> _requests;

  static int32_t readInt(std::istream& str) {
    int32_t val;
    str.read(reinterpret_cast<char*>(&val), sizeof(int32_t));
    return val;
  }

  static double readDouble(std::istream& str) {
    double val;
    str.read(reinterpret_cast<char*>(&val), sizeof(double));
    return val;
  }

  static std::string readString(std::istream& str) {
    int32_t length = readInt(str);
    if (length <= 0 || length >= 80) return std::string();
    std::string data(length, 0);
    str.read(&data[0], length);
    return std::string(&data[0]);
  }
};

}  // namespace rfiStrategy

#endif
