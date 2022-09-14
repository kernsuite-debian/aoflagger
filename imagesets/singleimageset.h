#ifndef SINGLEIMAGESET_H
#define SINGLEIMAGESET_H

#include <string>
#include <stdexcept>

#include "../structures/types.h"

#include "imageset.h"

namespace imagesets {

class SingleImageSet : public ImageSet {
 public:
  SingleImageSet()
      : ImageSet(), _readCount(0), _lastRead(nullptr), _writeFlagsIndex() {}

  std::string Name() const override = 0;
  size_t Size() const override { return 1; }
  std::string Description(const ImageSetIndex&) const override {
    return Name();
  }
  std::vector<std::string> Files() const override = 0;

  void AddReadRequest(const ImageSetIndex&) override {
    if (_lastRead != nullptr) {
      _lastRead.reset();
      _readCount = 1;
    } else {
      ++_readCount;
    }
  }
  void PerformReadRequests(class ProgressListener& progress) override {
    _lastRead = Read(progress);
    _lastRead->SetIndex(StartIndex());
  }

  std::unique_ptr<BaselineData> GetNextRequested() override {
    if (_readCount == 0)
      throw std::runtime_error("All data reads have already been requested");
    if (_lastRead == nullptr)
      throw std::runtime_error(
          "GetNextRequested() was called before PerformReadRequests()");
    return std::unique_ptr<BaselineData>(new BaselineData(*_lastRead));
  }

  virtual std::unique_ptr<BaselineData> Read(
      class ProgressListener& progress) = 0;

  virtual void Write(const std::vector<Mask2DCPtr>&) {
    throw std::runtime_error(
        "Flag writing is not implemented for this file (SingleImageSet)");
  }

  void AddWriteFlagsTask(const ImageSetIndex& index,
                         std::vector<Mask2DCPtr>& flags) override {
    _writeFlagsIndex = index;
    _writeFlagsMasks = flags;
  }

  void PerformWriteFlagsTask() override {
    if (_writeFlagsIndex.Empty()) throw std::runtime_error("Nothing to write");

    Write(_writeFlagsMasks);

    _writeFlagsIndex = ImageSetIndex();
  }
  virtual std::string BaselineDescription() = 0;
  bool HasCrossCorrelations() const override { return false; }

 protected:
  SingleImageSet(const SingleImageSet&)
      : _readCount(0),
        _lastRead(nullptr),
        _writeFlagsIndex(),
        _writeFlagsMasks() {}

 private:
  int _readCount;
  std::unique_ptr<BaselineData> _lastRead;
  ImageSetIndex _writeFlagsIndex;
  std::vector<Mask2DCPtr> _writeFlagsMasks;
};

}  // namespace imagesets

#endif
