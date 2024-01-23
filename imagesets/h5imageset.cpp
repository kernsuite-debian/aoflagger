#include "h5imageset.h"

#include "../util/logger.h"
#include "../util/progress/subtasklistener.h"

#include <H5Cpp.h>

#include <aocommon/uvector.h>

namespace imagesets {

namespace {
constexpr char kCoordinate0Path[] =
    "SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES/COORDINATE_0";
constexpr char kCoordinate1Path[] =
    "SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES/COORDINATE_1";
constexpr char kMainDataPath[] = "SUB_ARRAY_POINTING_000/BEAM_000/STOKES_0";
constexpr char kIncrementAttribute[] = "INCREMENT";
constexpr char kTelescopeAttribute[] = "TELESCOPE";
constexpr char kFrequencyAxisAttribute[] = "AXIS_VALUES_WORLD";

/**
 * Read the INCREMENT HDF5 field, which identified the seconds between
 * timesteps.
 */
double GetTimeIncrease(H5::H5File& file) {
  // Read the timestep increase
  const H5::Group group = file.openGroup(kCoordinate0Path);
  const H5::Attribute attribute = group.openAttribute(kIncrementAttribute);
  const H5::DataSpace space = attribute.getSpace();
  if (space.getSimpleExtentNdims() != 0)
    throw std::runtime_error("Invalid nr of dimensions (" +
                             std::to_string(space.getSimpleExtentNdims()) +
                             ") for attribute");
  double time_increase;
  attribute.read(H5::PredType::NATIVE_DOUBLE, &time_increase);
  return time_increase;
}

/**
 * Read the frequency axis, i.e., the frequency of every channel
 * (normally is uniform, but could theoretically not be the case)
 */
aocommon::UVector<double> GetFrequencyAxis(H5::H5File& file) {
  const H5::Group group = file.openGroup(kCoordinate1Path);
  const H5::Attribute attribute = group.openAttribute(kFrequencyAxisAttribute);
  const H5::DataSpace space = attribute.getSpace();
  if (space.getSimpleExtentNdims() != 1)
    throw std::runtime_error("Invalid nr of dimensions (" +
                             std::to_string(space.getSimpleExtentNdims()) +
                             ") for attribute");
  hsize_t freq_axis_size;
  space.getSimpleExtentDims(&freq_axis_size);
  aocommon::UVector<double> frequency_axis(freq_axis_size);
  attribute.read(H5::PredType::NATIVE_DOUBLE, frequency_axis.data());
  return frequency_axis;
}

/**
 * Read the "STOKES" field, transpose the data and return it
 * in a Image2D. An optional interval start and end can be specified
 * to limit reading from and/or up to some timestep. If either
 * one is not specified, the first and last timestep are used,
 * respectively.
 * @param range_start is the index of the first timestep after
 * having truncated the requested interval to the available times.
 */
Image2DPtr GetMainValues(H5::H5File& file, size_t& range_start,
                         std::optional<size_t> interval_start,
                         std::optional<size_t> interval_end) {
  const H5::DataSet data_set = file.openDataSet(kMainDataPath);
  const H5::DataSpace space = data_set.getSpace();
  std::vector<hsize_t> dims(space.getSimpleExtentNdims());
  if (dims.size() != 2)
    throw std::runtime_error("Invalid dimensionality of data");
  space.getSimpleExtentDims(dims.data());

  const size_t n_times = dims[0];
  const size_t n_channels = dims[1];
  range_start = std::min(interval_start.value_or(0), n_times);
  const size_t range_end =
      interval_end ? std::min(*interval_end, n_times) : n_times;

  dims[0] = range_end - range_start;
  const hsize_t mem_offsets[] = {0, 0};
  const H5::DataSpace memory_space(2, dims.data());
  memory_space.selectHyperslab(H5S_SELECT_SET, dims.data(), mem_offsets);
  const hsize_t file_offsets[] = {range_start, 0};
  const H5::DataSpace file_space(space);
  file_space.selectHyperslab(H5S_SELECT_SET, dims.data(), file_offsets);

  Image2DPtr image =
      Image2D::CreateUnsetImagePtr(n_channels, range_end - range_start);
  data_set.read(image->Data(), H5::PredType::NATIVE_FLOAT, memory_space,
                file_space);
  // In the H5 format, frequency is the fastest increasing axis. AOFlagger
  // expects this to be time:
  image->Transpose();
  return image;
}

}  // namespace

H5ImageSet::H5ImageSet(const std::string& path) : path_(path) {
  const H5::H5File file(path_, H5F_ACC_RDONLY);
  const H5::Attribute attribute = file.openAttribute(kTelescopeAttribute);
  attribute.read(H5::StrType(0, H5T_VARIABLE), telescope_name_);
}

void H5ImageSet::Initialize() {}

std::string H5ImageSet::Description(const ImageSetIndex&) const {
  return path_;
}

std::unique_ptr<BaselineData> H5ImageSet::LoadData(ProgressListener&,
                                                   const ImageSetIndex& index) {
  H5::H5File file(path_, H5F_ACC_RDONLY);
  const double time_increase = GetTimeIncrease(file);
  const aocommon::UVector<double> frequency_axis = GetFrequencyAxis(file);

  size_t range_start = 0;
  Image2DPtr image =
      GetMainValues(file, range_start, interval_start_, interval_end_);
  const size_t n_frequencies = image->Height();
  const size_t n_times = image->Width();
  if (n_frequencies != frequency_axis.size())
    throw std::runtime_error("Inconsistent axes in file");

  const TimeFrequencyData data(TimeFrequencyData::AmplitudePart,
                               aocommon::PolarizationEnum::StokesI,
                               std::move(image));

  // Fill the meta data structure
  TimeFrequencyMetaDataPtr meta_data(new TimeFrequencyMetaData());
  BandInfo band_info;
  band_info.channels.resize(n_frequencies);
  for (size_t ch = 0; ch != n_frequencies; ++ch) {
    band_info.channels[ch].frequencyHz = frequency_axis[ch];
    band_info.channels[ch].frequencyIndex = ch;
  }
  meta_data->SetBand(band_info);

  std::vector<double> times;
  times.reserve(n_times);
  for (size_t i = 0; i != n_times; ++i) {
    times.emplace_back(time_increase * (i + range_start));
  }
  meta_data->SetObservationTimes(std::move(times));

  return std::make_unique<BaselineData>(std::move(data), std::move(meta_data),
                                        index);
}

void H5ImageSet::PerformReadRequests(ProgressListener& progress) {
  progress.OnStartTask("Reading " + path_);
  for (size_t i = 0; i != requests_.size(); ++i) {
    const ImageSetIndex& index = requests_[i];
    SubTaskListener sub_listener(progress, i, requests_.size());
    baseline_buffer_.emplace(LoadData(sub_listener, index));
    progress.OnProgress(i + 1, requests_.size());
  }
  progress.OnFinish();
}

void H5ImageSet::AddWriteFlagsTask(const ImageSetIndex&,
                                   std::vector<Mask2DCPtr>&) {
  throw std::runtime_error(
      "Flags can not be written, H5 files do not support flags");
}

}  // namespace imagesets
