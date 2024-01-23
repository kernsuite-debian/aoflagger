#ifndef COADDED_IMAGE_SET_H
#define COADDED_IMAGE_SET_H

#include "imageset.h"
#include "msimageset.h"

#include "../structures/msmetadata.h"
#include "../util/logger.h"

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace imagesets {

using Sequence = MSMetaData::Sequence;

class CoaddedImageSet final : public IndexableSet {
 public:
  explicit CoaddedImageSet(const std::vector<std::string>& filenames,
                           BaselineIOMode ioMode) {
    if (filenames.empty())
      throw std::runtime_error(
          "Coadding of image sets was requested, but list of measurement sets "
          "to coadd is empty");
    if (filenames.size() == 1)
      throw std::runtime_error(
          "Coadding of image sets was requested, but only one measurement set "
          "is given");
    for (const std::string& path : filenames) {
      std::unique_ptr<MSImageSet> imageSet(new MSImageSet(path, ioMode));
      _msImageSets.emplace_back(std::move(imageSet));
    }
  }

  CoaddedImageSet(const CoaddedImageSet& source) = delete;
  CoaddedImageSet& operator=(const CoaddedImageSet& source) = delete;

  std::unique_ptr<ImageSet> Clone() override {
    std::unique_ptr<CoaddedImageSet> newSet(new CoaddedImageSet());
    for (const std::unique_ptr<MSImageSet>& imageSet : _msImageSets) {
      newSet->_msImageSets.emplace_back(
          std::unique_ptr<MSImageSet>(new MSImageSet(*imageSet)));
    }
    return newSet;
  }
  size_t Size() const override { return _msImageSets[0]->Size(); }
  std::string Description(const ImageSetIndex& index) const override;

  void Initialize() override {
    for (std::unique_ptr<MSImageSet>& imageSet : _msImageSets)
      imageSet->Initialize();
  }

  std::string Name() const override {
    return "Coadded set (" + _msImageSets.front()->Name() + " ...)";
  }
  std::vector<std::string> Files() const override {
    return _msImageSets.front()->Files();
  }

  void AddReadRequest(const ImageSetIndex& index) override {
    for (size_t i = 0; i != _msImageSets.size(); ++i) {
      _msImageSets[i]->AddReadRequest(index);
    }
  }

  void PerformReadRequests(class ProgressListener& progress) override {
    for (size_t i = 0; i != _msImageSets.size(); ++i) {
      _msImageSets[i]->PerformReadRequests(progress);  // TODO
    }
  }

  std::unique_ptr<BaselineData> GetNextRequested() override {
    std::unique_ptr<BaselineData> data =
        _msImageSets.front()->GetNextRequested();
    TimeFrequencyData tfData(data->Data());
    std::vector<Image2DPtr> images;
    for (size_t i = 0; i != tfData.PolarizationCount(); ++i) {
      TimeFrequencyData polAmplitude = tfData.MakeFromPolarizationIndex(i).Make(
          TimeFrequencyData::AmplitudePart);
      images.emplace_back(Image2DPtr(new Image2D(*polAmplitude.GetImage(0))));
    }
    for (size_t i = 1; i != _msImageSets.size(); ++i) {
      std::unique_ptr<BaselineData> addedData =
          _msImageSets[i]->GetNextRequested();
      if (addedData->Data().PolarizationCount() != images.size())
        throw std::runtime_error(
            "Coadded images have different number of polarizations");
      for (size_t j = 0; j != images.size(); ++j) {
        TimeFrequencyData polAmplitude =
            addedData->Data().MakeFromPolarizationIndex(j).Make(
                TimeFrequencyData::AmplitudePart);
        images[j]->operator+=(*polAmplitude.GetImage(0));
      }
    }
    Image2DCPtr zeroImage = Image2D::CreateSetImagePtr(
        images.front()->Width(), images.front()->Height(), 0.0);
    for (size_t i = 0; i != tfData.PolarizationCount(); ++i) {
      TimeFrequencyData pol = tfData.MakeFromPolarizationIndex(i);
      pol.SetImage(0, images[i]);
      pol.SetImage(1, zeroImage);
      tfData.SetPolarizationData(i, std::move(pol));
    }
    data->SetData(tfData);
    return data;
  }

  void AddWriteFlagsTask(const ImageSetIndex& index,
                         std::vector<Mask2DCPtr>& flags) override {}
  void PerformWriteFlagsTask() override {}

  BaselineReaderPtr Reader() const override {
    return _msImageSets.front()->Reader();
  }

  size_t AntennaCount() const override {
    return _msImageSets.front()->AntennaCount();
  }

  size_t GetAntenna1(const ImageSetIndex& index) const override {
    return _msImageSets.front()->GetAntenna1(index);
  }

  size_t GetAntenna2(const ImageSetIndex& index) const override {
    return _msImageSets.front()->GetAntenna2(index);
  }

  size_t GetBand(const ImageSetIndex& index) const override {
    return _msImageSets.front()->GetBand(index);
  }

  size_t GetField(const ImageSetIndex& index) const override {
    return _msImageSets.front()->GetField(index);
  }

  size_t GetSequenceId(const ImageSetIndex& index) const override {
    return _msImageSets.front()->GetSequenceId(index);
  }

  AntennaInfo GetAntennaInfo(unsigned antennaIndex) const override {
    return _msImageSets.front()->GetAntennaInfo(antennaIndex);
  }

  size_t BandCount() const override {
    return _msImageSets.front()->BandCount();
  }

  BandInfo GetBandInfo(unsigned bandIndex) const override {
    return _msImageSets.front()->GetBandInfo(bandIndex);
  }

  size_t SequenceCount() const override {
    return _msImageSets.front()->SequenceCount();
  }

  std::optional<ImageSetIndex> Index(size_t antenna1, size_t antenna2,
                                     size_t bandIndex,
                                     size_t sequenceId) const override {
    return _msImageSets[0]->Index(antenna1, antenna2, bandIndex, sequenceId);
  }

  FieldInfo GetFieldInfo(unsigned fieldIndex) const override {
    return _msImageSets.front()->GetFieldInfo(fieldIndex);
  }

  const std::vector<std::unique_ptr<MSImageSet>>& MSImageSets() {
    return _msImageSets;
  }

 private:
  CoaddedImageSet() {}

  std::vector<std::unique_ptr<MSImageSet>> _msImageSets;
};

std::string CoaddedImageSet::Description(const ImageSetIndex& index) const {
  return _msImageSets[0]->Description(index) + " (coadded)";
}
}  // namespace imagesets

#endif
