#ifndef AOFLAGGER_ALGORITHMS_RESAMPLING_H
#define AOFLAGGER_ALGORITHMS_RESAMPLING_H

#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

#include "thresholdtools.h"

namespace algorithms {

void downsample_masked(TimeFrequencyData& tfData,
                       TimeFrequencyMetaData* metaData, size_t horizontalFactor,
                       size_t verticalFactor) {
  // Decrease in horizontal direction
  size_t polCount = tfData.PolarizationCount();
  for (size_t i = 0; i < polCount; ++i) {
    TimeFrequencyData polData(tfData.MakeFromPolarizationIndex(i));
    const Mask2DCPtr mask = polData.GetSingleMask();
    for (unsigned j = 0; j < polData.ImageCount(); ++j) {
      const Image2DCPtr image = polData.GetImage(j);
      polData.SetImage(j, ThresholdTools::ShrinkHorizontally(
                              horizontalFactor, image.get(), mask.get()));
    }
    tfData.SetPolarizationData(i, std::move(polData));
  }
  size_t maskCount = tfData.MaskCount();
  for (size_t i = 0; i < maskCount; ++i) {
    Mask2DCPtr mask = tfData.GetMask(i);
    Mask2DPtr newMask(
        new Mask2D(mask->ShrinkHorizontallyForAveraging(horizontalFactor)));
    tfData.SetMask(i, std::move(newMask));
  }

  // Decrease in vertical direction
  for (size_t i = 0; i < polCount; ++i) {
    TimeFrequencyData polData(tfData.MakeFromPolarizationIndex(i));
    const Mask2DCPtr mask = polData.GetSingleMask();
    for (unsigned j = 0; j < polData.ImageCount(); ++j) {
      const Image2DCPtr image = polData.GetImage(j);
      polData.SetImage(j, ThresholdTools::ShrinkVertically(
                              verticalFactor, image.get(), mask.get()));
    }
    tfData.SetPolarizationData(i, std::move(polData));
  }
  for (size_t i = 0; i < maskCount; ++i) {
    Mask2DCPtr mask = tfData.GetMask(i);
    Mask2DPtr newMask(
        new Mask2D(mask->ShrinkVerticallyForAveraging(verticalFactor)));
    tfData.SetMask(i, std::move(newMask));
  }

  if (metaData) {
    if (metaData->HasBand() && verticalFactor != 1) {
      BandInfo newBand = metaData->Band();
      size_t newNChannels = tfData.ImageHeight();
      newBand.channels.resize(newNChannels);
      for (size_t i = 0; i != newNChannels; ++i) {
        const size_t startChannel = i * verticalFactor;
        const size_t endChannel = std::min((i + 1) * verticalFactor,
                                           metaData->Band().channels.size());
        double f = 0;
        for (size_t j = startChannel; j != endChannel; ++j) {
          f += metaData->Band().channels[j].frequencyHz;
        }
        newBand.channels[i].frequencyHz = f / (endChannel - startChannel);
      }
      metaData->SetBand(newBand);
    }
    if (metaData->HasObservationTimes() && horizontalFactor != 1) {
      size_t newNTimes = tfData.ImageWidth();
      std::vector<double> times(newNTimes);
      for (size_t i = 0; i != newNTimes; ++i) {
        const size_t startTime = i * horizontalFactor;
        const size_t endTime = std::min((i + 1) * horizontalFactor,
                                        metaData->ObservationTimes().size());
        double t = 0;
        for (size_t j = startTime; j != endTime; ++j) {
          t += metaData->ObservationTimes()[j];
        }
        times[i] = t / (endTime - startTime);
      }
      metaData->SetObservationTimes(times);
    }
  }
}

void upsample_image(TimeFrequencyData timeFrequencyData,
                    TimeFrequencyData& destination, size_t horizontalFactor,
                    size_t verticalFactor) {
  const size_t imageCount = timeFrequencyData.ImageCount(),
               newWidth = destination.ImageWidth(),
               newHeight = destination.ImageHeight();
  if (destination.ImageCount() != imageCount)
    throw std::runtime_error(
        "Error in upsample() call: source and image have different number of "
        "images");

  if (horizontalFactor > 1) {
    for (size_t i = 0; i < imageCount; ++i) {
      Image2DPtr newImage(
          new Image2D(timeFrequencyData.GetImage(i)->EnlargeHorizontally(
              horizontalFactor, newWidth)));
      timeFrequencyData.SetImage(i, newImage);
    }
  }

  for (size_t i = 0; i < imageCount; ++i) {
    if (verticalFactor > 1) {
      Image2DPtr newImage(
          new Image2D(timeFrequencyData.GetImage(i)->EnlargeVertically(
              verticalFactor, newHeight)));
      destination.SetImage(i, newImage);
    } else {
      destination.SetImage(i, timeFrequencyData.GetImage(i));
    }
  }
}

void upsample_mask(TimeFrequencyData timeFrequencyData,
                   TimeFrequencyData& destination, size_t horizontalFactor,
                   size_t verticalFactor) {
  const size_t maskCount = timeFrequencyData.MaskCount(),
               newWidth = destination.ImageWidth(),
               newHeight = destination.ImageHeight(),
               oldHeight = timeFrequencyData.ImageHeight();
  if (destination.MaskCount() != maskCount)
    throw std::runtime_error(
        "Error in upsample() call: source and image have different number of "
        "masks");

  if (horizontalFactor > 1) {
    for (size_t i = 0; i != maskCount; ++i) {
      Mask2DPtr newMask = Mask2D::CreateUnsetMaskPtr(newWidth, oldHeight);
      newMask->EnlargeHorizontallyAndSet(*timeFrequencyData.GetMask(i),
                                         horizontalFactor);
      timeFrequencyData.SetMask(i, newMask);
    }
  }

  for (size_t i = 0; i != maskCount; ++i) {
    if (verticalFactor > 1) {
      Mask2DPtr newMask = Mask2D::CreateUnsetMaskPtr(newWidth, newHeight);
      newMask->EnlargeVerticallyAndSet(*timeFrequencyData.GetMask(i),
                                       verticalFactor);
      destination.SetMask(i, newMask);
    } else {
      destination.SetMask(i, timeFrequencyData.GetMask(i));
    }
  }
}

}  // namespace algorithms

#endif
