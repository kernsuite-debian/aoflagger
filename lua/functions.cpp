#include "functions.h"

#include "scriptdata.h"

#include "../algorithms/applybandpass.h"
#include "../algorithms/highpassfilter.h"
#include "../algorithms/medianwindow.h"
#include "../algorithms/resampling.h"
#include "../algorithms/siroperator.h"
#include "../algorithms/thresholdconfig.h"

#include "../structures/image2d.h"
#include "../structures/samplerow.h"
#include "../structures/timefrequencydata.h"

#ifdef HAVE_GTKMM
#include "../rfigui/maskedheatmap.h"
#endif

#include "../algorithms/polarizationstatistics.h"
#include "../algorithms/thresholdtools.h"

#include "../quality/statisticscollection.h"

#include <iostream>

using algorithms::ApplyBandpass;
using algorithms::HighPassFilter;
using algorithms::MedianWindow;
using algorithms::PolarizationStatistics;
using algorithms::SIROperator;
using algorithms::ThresholdConfig;
using algorithms::ThresholdTools;

namespace aoflagger_lua {

void apply_bandpass(Data& data, const std::string& filename,
                    ScriptData& scriptData) {
  std::unique_ptr<BandpassFile>& bpFile = scriptData.GetBandpassFile();
  {
    std::lock_guard<std::mutex> lock(scriptData.BandpassMutex());
    if (bpFile == nullptr) {
      bpFile.reset(new BandpassFile(filename));
    }
  }
  ApplyBandpass::Apply(data.TFData(), *bpFile, data.MetaData()->Antenna1().name,
                       data.MetaData()->Antenna2().name);
}

void collect_statistics(const Data& dataAfter, const Data& dataBefore,
                        ScriptData& scriptData) {
  std::unique_ptr<StatisticsCollection>& statistics(scriptData.GetStatistics());
  size_t polarizationCount = dataAfter.TFData().PolarizationCount();
  if (dataBefore.TFData().PolarizationCount() != polarizationCount)
    throw std::runtime_error(
        "Before and after have different nr of polarizations in call to "
        "collect_statistics()");
  if (dataAfter.TFData().ComplexRepresentation() !=
      TimeFrequencyData::ComplexParts)
    throw std::runtime_error(
        "collect_statistics(): statistics can only be collected for complex "
        "data, first parameter is not complex");
  if (dataBefore.TFData().ComplexRepresentation() !=
      TimeFrequencyData::ComplexParts)
    throw std::runtime_error(
        "collect_statistics(): statistics can only be collected for complex "
        "data, second parameter is not complex");
  if (!dataBefore.MetaData())
    throw std::runtime_error("collect_statistics(): missing metadata");
  if (!dataBefore.MetaData()->HasBand())
    throw std::runtime_error("collect_statistics(): missing band metadata");
  if (!statistics)
    statistics.reset(new StatisticsCollection(polarizationCount));
  size_t bandIndex = dataBefore.MetaData()->Band().windowIndex;
  if (!statistics->HasBand(bandIndex)) {
    std::vector<double> channels(dataBefore.MetaData()->Band().channels.size());
    for (size_t i = 0; i != channels.size(); ++i)
      channels[i] = dataBefore.MetaData()->Band().channels[i].frequencyHz;
    statistics->InitializeBand(bandIndex, channels.data(), channels.size());
  }
  bool useEmpty = (dataBefore.TFData().MaskCount() == 0 ||
                   dataAfter.TFData().MaskCount() == 0);
  Mask2DPtr emptyMask;
  if (useEmpty) {
    // TODO we can avoid this allocation when StatisticsCollection::AddImage()
    // would support a call without a 2nd mask
    emptyMask = Mask2D::CreateSetMaskPtr<false>(
        dataBefore.TFData().ImageWidth(), dataBefore.TFData().ImageHeight());
  }
  if (!dataAfter.MetaData()->HasAntenna1() ||
      !dataAfter.MetaData()->HasAntenna2() ||
      !dataAfter.MetaData()->HasObservationTimes())
    throw std::runtime_error(
        "collect_statistics(): can't collect statistics for sets without "
        "metadata (antenna info, time info)");
  size_t antenna1 = dataAfter.MetaData()->Antenna1().id,
         antenna2 = dataAfter.MetaData()->Antenna2().id;
  const std::vector<double>& times = dataAfter.MetaData()->ObservationTimes();
  for (size_t polarization = 0; polarization != polarizationCount;
       ++polarization) {
    TimeFrequencyData polDataBefore =
                          dataBefore.TFData().MakeFromPolarizationIndex(
                              polarization),
                      polDataAfter =
                          dataAfter.TFData().MakeFromPolarizationIndex(
                              polarization);

    Mask2DCPtr beforeMask, afterMask;
    if (dataBefore.TFData().MaskCount() == 0)
      beforeMask = emptyMask;
    else
      beforeMask = polDataBefore.GetSingleMask();
    if (dataAfter.TFData().MaskCount() == 0)
      afterMask = emptyMask;
    else
      afterMask = polDataAfter.GetSingleMask();

    statistics->AddImage(antenna1, antenna2, &times[0], bandIndex, polarization,
                         polDataBefore.GetRealPart(),
                         polDataBefore.GetImaginaryPart(), afterMask,
                         beforeMask);
  }
}

void copy_to_channel(Data& destination, const Data& source, size_t channel) {
  if (channel >= destination.TFData().ImageHeight())
    throw std::runtime_error(
        "copy_to_channel(): channel parameter is outside the band");
  destination.TFData().CopyFrom(source.TFData(), 0, channel);
}

void copy_to_frequency(Data& destination, const Data& source,
                       double frequencyHz) {
  if (destination.MetaData() == nullptr || !destination.MetaData()->HasBand())
    throw std::runtime_error(
        "copy_to_frequency(): no frequency meta data available in data object");
  const BandInfo& band = destination.MetaData()->Band();
  ChannelInfo channel;
  channel.frequencyHz = frequencyHz;
  std::vector<ChannelInfo>::const_iterator iter;
  if (band.channels.begin() > band.channels.end()) {
    iter = std::lower_bound(
        band.channels.begin(), band.channels.end(), channel,
        [](const ChannelInfo& lhs, const ChannelInfo& rhs) -> bool {
          return lhs.frequencyHz > rhs.frequencyHz;
        });
  } else {
    iter = std::lower_bound(
        band.channels.begin(), band.channels.end(), channel,
        [](const ChannelInfo& lhs, const ChannelInfo& rhs) -> bool {
          return lhs.frequencyHz < rhs.frequencyHz;
        });
  }
  const size_t channelIndex = iter - band.channels.begin();
  copy_to_channel(destination, source, channelIndex);
}

void upsample_image(const Data& input, Data& destination,
                    size_t horizontalFactor, size_t verticalFactor) {
  algorithms::upsample_image(input.TFData(), destination.TFData(),
                             horizontalFactor, verticalFactor);
}

void upsample_mask(const Data& input, Data& destination,
                   size_t horizontalFactor, size_t verticalFactor) {
  algorithms::upsample_mask(input.TFData(), destination.TFData(),
                            horizontalFactor, verticalFactor);
}

void low_pass_filter(Data& data, size_t kernelWidth, size_t kernelHeight,
                     double horizontalSigmaSquared,
                     double verticalSigmaSquared) {
  if (data.TFData().PolarizationCount() != 1)
    throw std::runtime_error("High-pass filtering needs single polarization");
  HighPassFilter filter;
  filter.SetHWindowSize(kernelWidth);
  filter.SetVWindowSize(kernelHeight);
  filter.SetHKernelSigmaSq(horizontalSigmaSquared);
  filter.SetVKernelSigmaSq(verticalSigmaSquared);
  Mask2DCPtr mask = data.TFData().GetSingleMask();
  size_t imageCount = data.TFData().ImageCount();

  for (size_t i = 0; i < imageCount; ++i)
    data.TFData().SetImage(
        i, filter.ApplyLowPass(data.TFData().GetImage(i), mask));
}

void high_pass_filter(Data& data, size_t kernelWidth, size_t kernelHeight,
                      double horizontalSigmaSquared,
                      double verticalSigmaSquared) {
  if (data.TFData().PolarizationCount() != 1)
    throw std::runtime_error("High-pass filtering needs single polarization");
  HighPassFilter filter;
  filter.SetHWindowSize(kernelWidth);
  filter.SetVWindowSize(kernelHeight);
  filter.SetHKernelSigmaSq(horizontalSigmaSquared);
  filter.SetVKernelSigmaSq(verticalSigmaSquared);
  Mask2DCPtr mask = data.TFData().GetSingleMask();
  size_t imageCount = data.TFData().ImageCount();

  for (size_t i = 0; i < imageCount; ++i)
    data.TFData().SetImage(
        i, filter.ApplyHighPass(data.TFData().GetImage(i), mask));
}

void save_heat_map(const char* filename, const Data& data) {
#ifdef HAVE_GTKMM
  const TimeFrequencyData tfData = data.TFData();
  MaskedHeatMap plot;
  plot.SetImage(
      std::unique_ptr<PlotImage>(new PlotImage(tfData.GetSingleImage())));
  plot.SetAlternativeMask(tfData.GetSingleMask());
  plot.SaveByExtension(filename, 800, 500);
#else
  throw std::runtime_error("Compiled without GTKMM -- can not save heat map");
#endif
}

void print_polarization_statistics(const Data& data) {
  PolarizationStatistics statistics;
  statistics.Add(data.TFData());
  statistics.Report();
}

void scale_invariant_rank_operator(Data& data, double level_horizontal,
                                   double level_vertical) {
  if (!data.TFData().IsEmpty()) {
    Mask2DPtr mask(new Mask2D(*data.TFData().GetSingleMask()));

    SIROperator::OperateHorizontally(*mask, level_horizontal);
    SIROperator::OperateVertically(*mask, level_vertical);
    data.TFData().SetGlobalMask(mask);
  }
}

void scale_invariant_rank_operator_masked(Data& data, const Data& missing,
                                          double level_horizontal,
                                          double level_vertical,
                                          double penalty) {
  if (!data.TFData().IsEmpty()) {
    Mask2DPtr mask(new Mask2D(*data.TFData().GetSingleMask()));

    Mask2DCPtr missingMask = missing.TFData().GetSingleMask();
    SIROperator::OperateHorizontallyMissing(*mask, *missingMask,
                                            level_horizontal, penalty);
    SIROperator::OperateVerticallyMissing(*mask, *missingMask, level_vertical,
                                          penalty);
    data.TFData().SetGlobalMask(mask);
  }
}

Data downsample(const Data& data, size_t horizontalFactor,
                size_t verticalFactor) {
  TimeFrequencyData timeFrequencyData = data.TFData();
  const size_t imageCount = timeFrequencyData.ImageCount();
  const size_t maskCount = timeFrequencyData.MaskCount();

  if (horizontalFactor > 1) {
    for (size_t i = 0; i < imageCount; ++i) {
      Image2DPtr newImage(new Image2D(
          timeFrequencyData.GetImage(i)->ShrinkHorizontally(horizontalFactor)));
      timeFrequencyData.SetImage(i, newImage);
    }
    for (size_t i = 0; i < maskCount; ++i) {
      Mask2DPtr newMask(new Mask2D(
          timeFrequencyData.GetMask(i)->ShrinkHorizontally(horizontalFactor)));
      timeFrequencyData.SetMask(i, newMask);
    }
  }

  if (verticalFactor > 1) {
    for (size_t i = 0; i < imageCount; ++i) {
      Image2DPtr newImage(new Image2D(
          timeFrequencyData.GetImage(i)->ShrinkVertically(verticalFactor)));
      timeFrequencyData.SetImage(i, newImage);
    }
    for (size_t i = 0; i < maskCount; ++i) {
      Mask2DPtr newMask(new Mask2D(
          timeFrequencyData.GetMask(i)->ShrinkVertically(verticalFactor)));
      timeFrequencyData.SetMask(i, newMask);
    }
  }
  return Data(timeFrequencyData, data.MetaData(), data.GetContext());
}

Data downsample_masked(const Data& data, size_t horizontalFactor,
                       size_t verticalFactor) {
  TimeFrequencyData timeFrequencyData = data.TFData();
  TimeFrequencyMetaDataPtr metaData;
  if (data.MetaData()) {
    metaData.reset(new TimeFrequencyMetaData(*data.MetaData()));
    algorithms::downsample_masked(timeFrequencyData, metaData.get(),
                                  horizontalFactor, verticalFactor);
  } else {
    algorithms::downsample_masked(timeFrequencyData, nullptr, horizontalFactor,
                                  verticalFactor);
  }
  return Data(timeFrequencyData, metaData, data.GetContext());
}

static void sumthreshold_generic(Data& data, const Data* missing,
                                 double hThresholdFactor,
                                 double vThresholdFactor, bool horizontal,
                                 bool vertical) {
  ThresholdConfig thresholdConfig;
  thresholdConfig.InitializeLengthsDefault();
  thresholdConfig.InitializeThresholdsFromFirstThreshold(
      6.0L, ThresholdConfig::Rayleigh);
  if (!horizontal) thresholdConfig.RemoveHorizontalOperations();
  if (!vertical) thresholdConfig.RemoveVerticalOperations();

  if (data.TFData().PolarizationCount() != 1)
    throw std::runtime_error("Input data in sum_threshold has wrong format");

  Mask2DPtr mask(new Mask2D(*data.TFData().GetSingleMask()));
  Image2DCPtr image = data.TFData().GetSingleImage();

  if (missing != nullptr) {
    Mask2DCPtr missingMask = missing->TFData().GetSingleMask();
    thresholdConfig.ExecuteWithMissing(image.get(), mask.get(),
                                       missingMask.get(), false,
                                       hThresholdFactor, vThresholdFactor);
  } else {
    thresholdConfig.Execute(image.get(), mask.get(), false, hThresholdFactor,
                            vThresholdFactor);
  }
  data.TFData().SetGlobalMask(mask);
}

void sumthreshold(Data& data, double hThresholdFactor, double vThresholdFactor,
                  bool horizontal, bool vertical) {
  sumthreshold_generic(data, nullptr, hThresholdFactor, vThresholdFactor,
                       horizontal, vertical);
}

void sumthreshold_masked(Data& data, const Data& missing,
                         double hThresholdFactor, double vThresholdFactor,
                         bool horizontal, bool vertical) {
  sumthreshold_generic(data, &missing, hThresholdFactor, vThresholdFactor,
                       horizontal, vertical);
}

void threshold_channel_rms(Data& data, double threshold,
                           bool thresholdLowValues) {
  Image2DCPtr image(data.TFData().GetSingleImage());
  SampleRow channels = SampleRow::MakeEmpty(image->Height());
  Mask2DPtr mask(new Mask2D(*data.TFData().GetSingleMask()));
  for (size_t y = 0; y < image->Height(); ++y) {
    SampleRow row =
        SampleRow::MakeFromRowWithMissings(image.get(), mask.get(), y);
    channels.SetValue(y, row.RMSWithMissings());
  }
  bool change;
  do {
    num_t median = channels.MedianWithMissings();
    num_t stddev = channels.StdDevWithMissings(median);
    change = false;
    double effectiveThreshold = threshold * stddev;
    for (size_t y = 0; y < channels.Size(); ++y) {
      if (!channels.ValueIsMissing(y) &&
          (channels.Value(y) - median > effectiveThreshold ||
           (thresholdLowValues &&
            median - channels.Value(y) > effectiveThreshold))) {
        mask->SetAllHorizontally<true>(y);
        channels.SetValueMissing(y);
        change = true;
      }
    }
  } while (change);
  data.TFData().SetGlobalMask(std::move(mask));
}

void threshold_timestep_rms(Data& data, double threshold) {
  if (!data.TFData().IsEmpty()) {
    Image2DCPtr image = data.TFData().GetSingleImage();
    SampleRow timesteps = SampleRow::MakeEmpty(image->Width());
    Mask2DPtr mask(new Mask2D(*data.TFData().GetSingleMask()));
    for (size_t x = 0; x < image->Width(); ++x) {
      SampleRow row =
          SampleRow::MakeFromColumnWithMissings(image.get(), mask.get(), x);
      timesteps.SetValue(x, row.RMSWithMissings());
    }
    bool change;
    MedianWindow<num_t>::SubtractMedian(timesteps, 511);
    do {
      num_t median = 0.0;
      num_t stddev = timesteps.StdDevWithMissings(0.0);
      change = false;
      for (size_t x = 0; x < timesteps.Size(); ++x) {
        if (!timesteps.ValueIsMissing(x) &&
            (timesteps.Value(x) - median > stddev * threshold ||
             median - timesteps.Value(x) > stddev * threshold)) {
          mask->SetAllVertically<true>(x);
          timesteps.SetValueMissing(x);
          change = true;
        }
      }
    } while (change);
    data.TFData().SetGlobalMask(std::move(mask));
  }
}

Data trim_channels(const Data& data, size_t start_channel, size_t end_channel) {
  if (start_channel > data.TFData().ImageHeight())
    throw std::runtime_error("trim_channels(): Invalid start channel");
  if (end_channel > data.TFData().ImageHeight())
    throw std::runtime_error("trim_channels(): Invalid end channel");
  if (start_channel >= end_channel)
    throw std::runtime_error("trim_channels(): Invalid range (start >= end)");
  TimeFrequencyData trimmedData = data.TFData();
  trimmedData.Trim(0, start_channel, trimmedData.ImageWidth(), end_channel);
  if (data.MetaData()) {
    TimeFrequencyMetaDataPtr metaData(
        new TimeFrequencyMetaData(*data.MetaData()));
    if (metaData->HasBand()) {
      // Correct the band data
      BandInfo band = metaData->Band();
      band.channels.assign(
          data.MetaData()->Band().channels.begin() + start_channel,
          data.MetaData()->Band().channels.begin() + end_channel);
      metaData->SetBand(band);
    }
    return Data(std::move(trimmedData), metaData, data.GetContext());
  } else {
    return Data(std::move(trimmedData), nullptr, data.GetContext());
  }
}

Data trim_frequencies(const Data& data, double start_frequency,
                      double end_frequency) {
  if (start_frequency >= end_frequency)
    throw std::runtime_error(
        "trim_frequencies(): Invalid range (start >= end)");
  if (data.MetaData() != nullptr && data.MetaData()->HasBand()) {
    std::pair<size_t, size_t> channelRange =
        data.MetaData()->Band().GetChannelRange(start_frequency, end_frequency);
    return trim_channels(data, channelRange.first, channelRange.second);
  } else
    throw std::runtime_error(
        "trim_frequency(): No spectral band information available!");
}

void visualize(Data& data, const std::string& label, size_t sortingIndex,
               ScriptData& scriptData) {
  scriptData.AddVisualization(data.TFData(), label, sortingIndex);
}

}  // namespace aoflagger_lua
