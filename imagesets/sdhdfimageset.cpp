#include "sdhdfimageset.h"

#include <H5Cpp.h>

#include "../util/logger.h"
#include "../util/progress/subtasklistener.h"

#include "aocommon/uvector.h"

namespace imagesets {

struct ObsParams {
  double mjd;
};

SdhdfImageSet::SdhdfImageSet(const std::string& path) : _path(path) {
  const H5::H5File file(_path, H5F_ACC_RDONLY);
  const H5::DataSet headerSet = file.openDataSet("metadata/primary_header");
  const H5::DataSpace headerSpace = headerSet.getSpace();
  if (headerSpace.getSimpleExtentNdims() != 1)
    throw std::runtime_error(
        "Incorrect dimensionality of metadata/primary_header");
  hsize_t dims;
  headerSpace.getSimpleExtentDims(&dims);
  if (dims != 1)
    throw std::runtime_error(
        "Incorrect nr of elements in metadata/primary_header data set");

  const H5::CompType headerType(sizeof(Header));
  headerType.insertMember("TELESCOPE", HOFFSET(Header, telescopeName),
                          H5::StrType(H5::PredType::C_S1, 64));
  headerType.insertMember("N_BEAMS", HOFFSET(Header, nBeams),
                          H5::PredType::NATIVE_INT);
  Header header;
  headerSet.read(&header, headerType);
  _beams.resize(header.nBeams);

  const H5::CompType bandParamsType(sizeof(BandParams));
  bandParamsType.insertMember("LABEL", HOFFSET(BandParams, label),
                              H5::StrType(H5::PredType::C_S1, 64));
  bandParamsType.insertMember("CENTRE_FREQ",
                              HOFFSET(BandParams, centreFrequency),
                              H5::PredType::NATIVE_DOUBLE);
  bandParamsType.insertMember("LOW_FREQ", HOFFSET(BandParams, lowFrequency),
                              H5::PredType::NATIVE_DOUBLE);
  bandParamsType.insertMember("HIGH_FREQ", HOFFSET(BandParams, highFrequency),
                              H5::PredType::NATIVE_DOUBLE);
  bandParamsType.insertMember("N_CHANS", HOFFSET(BandParams, nChannels),
                              H5::PredType::NATIVE_INT);
  bandParamsType.insertMember("N_POLS", HOFFSET(BandParams, nPolarizations),
                              H5::PredType::NATIVE_INT);

  _telescopeName = header.telescopeName;
  Logger::Debug << "Opening sdhdf file for telescope " << header.telescopeName
                << " with " << _beams.size() << " beam(s).\n";

  for (size_t beamIndex = 0; beamIndex != _beams.size(); ++beamIndex) {
    const H5::DataSet beamSet = file.openDataSet(
        "beam_" + std::to_string(beamIndex) + "/metadata/band_params");
    const H5::DataSpace space = beamSet.getSpace();
    std::vector<hsize_t> dims(space.getSimpleExtentNdims());
    if (dims.size() != 1)
      throw std::runtime_error("Invalid dimensionality of band_params");
    space.getSimpleExtentDims(dims.data());

    const size_t nBands = dims[0];
    Beam& beam = _beams[beamIndex];
    beam.bandParams.resize(nBands);

    beamSet.read(beam.bandParams.data(), bandParamsType);

    for (size_t bandIndex = 0; bandIndex != nBands; ++bandIndex)
      _indexTable.emplace_back(beamIndex, bandIndex);
  }
}

SdhdfImageSet::~SdhdfImageSet() {}

void SdhdfImageSet::Initialize() {}

std::string SdhdfImageSet::Description(const ImageSetIndex& index) const {
  const size_t beam = _indexTable[index.Value()].first;
  const size_t band = _indexTable[index.Value()].second;
  return "Beam " + std::to_string(beam) + ", " +
         _beams[beam].bandParams[band].label;
}

bool SdhdfImageSet::tryOpen(H5::DataSet& dataSet, H5::H5File& file,
                            const std::string& name) {
#if H5_VERSION_GE(1, 10, 7)
  if (file.exists(name)) {
    dataSet = file.openDataSet(name);
    return true;
  } else {
    return false;
  }
#else
  // file.exists(name) doesn't work in older HDF5 libs, so use trial
  // and error. Unfortunately this generates output on the cmdline
  // when the file does not exist, hence only use it as fall back.
  try {
    dataSet = file.openDataSet(name);
    return true;
  } catch (H5::FileIException&) {
    // The data set did not exist (probably)
    return false;
  }
#endif
}

BaselineData SdhdfImageSet::loadData(ProgressListener& progress,
                                     const ImageSetIndex& index) {
  const size_t beam = _indexTable[index.Value()].first;
  const size_t band = _indexTable[index.Value()].second;
  const BandParams& bandParams = _beams[beam].bandParams[band];
  const std::string label = bandParams.label;
  Logger::Info << "Reading beam " << beam << ", " << label << "...\n";
  H5::H5File file(_path, H5F_ACC_RDONLY);

  const std::string beamPrefix = "beam_" + std::to_string(beam) + "/" + label;
  const std::string dataPrefix = beamPrefix + "/astronomy_data/";
  const H5::DataSet dataSet = file.openDataSet(dataPrefix + "data");
  const H5::DataSpace space = dataSet.getSpace();
  std::vector<hsize_t> ddims(space.getSimpleExtentNdims());
  space.getSimpleExtentDims(ddims.data());
  // should be time, beam, polarization, frequency, bin
  if (ddims.size() != 5)
    throw std::runtime_error("Incorrect dimensionality of main data array");
  const size_t nTimes = ddims[0];
  if (ddims[1] != 1)
    throw std::runtime_error(
        "Beam axis size is not one, don't know how to handle this file");
  if (int(ddims[2]) != bandParams.nPolarizations)
    throw std::runtime_error("Polarization axis has incorrect size");
  if (int(ddims[3]) != bandParams.nChannels)
    throw std::runtime_error("Frequency axis has incorrect size");
  if (ddims[4] != 1)
    throw std::runtime_error(
        "bin axis size is not one, don't know how to handle this file");
  aocommon::UVector<float> data(nTimes * bandParams.nPolarizations *
                                bandParams.nChannels);
  progress.OnProgress(1, 10);
  dataSet.read(data.data(), H5::PredType::NATIVE_FLOAT);

  std::vector<Image2DPtr> images(bandParams.nPolarizations);
  for (int pol = 0; pol != bandParams.nPolarizations; ++pol)
    images[pol] = Image2D::CreateUnsetImagePtr(nTimes, bandParams.nChannels);
  const float* valuePtr = data.data();
  for (size_t timeIndex = 0; timeIndex != nTimes; ++timeIndex) {
    for (int pol = 0; pol != bandParams.nPolarizations; ++pol) {
      for (int channel = 0; channel != bandParams.nChannels; ++channel) {
        images[pol]->SetValue(timeIndex, channel, *valuePtr);
        ++valuePtr;
      }
    }
  }

  Mask2DPtr mask;
  H5::DataSet flagsSet;
  if (tryOpen(flagsSet, file, dataPrefix + "flag")) {
    progress.OnProgress(5, 10);
    Logger::Info << "Reading flags...\n";
    const H5::DataSpace space = flagsSet.getSpace();
    std::vector<hsize_t> fdims(space.getSimpleExtentNdims());
    space.getSimpleExtentDims(fdims.data());
    // should be time, frequency
    if (fdims.size() != 2)
      throw std::runtime_error("Incorrect dimensionality of main flag array");
    const size_t nTimes = fdims[0];
    if (size_t(fdims[0]) != nTimes)
      throw std::runtime_error(
          "Polarization axis of flag data has incorrect size");
    if (int(fdims[1]) != bandParams.nChannels)
      throw std::runtime_error(
          "Frequency axis of flag data has incorrect size");
    aocommon::UVector<unsigned char> flags(nTimes * bandParams.nChannels);
    progress.OnProgress(6, 10);
    flagsSet.read(flags.data(), H5::PredType::NATIVE_UINT8);

    mask = Mask2D::CreateUnsetMaskPtr(nTimes, bandParams.nChannels);
    const unsigned char* valuePtr = flags.data();
    for (size_t timeIndex = 0; timeIndex != nTimes; ++timeIndex) {
      for (int channel = 0; channel != bandParams.nChannels; ++channel) {
        mask->SetValue(timeIndex, channel, *valuePtr);
        ++valuePtr;
      }
    }
  }
  progress.OnProgress(9, 10);

  TimeFrequencyData tfData;
  if (bandParams.nPolarizations == 4)
    tfData = TimeFrequencyData::FromLinear(
        TimeFrequencyData::AmplitudePart, std::move(images[0]),
        std::move(images[2]), std::move(images[3]), std::move(images[1]));
  else if (bandParams.nPolarizations == 2)
    tfData = TimeFrequencyData(TimeFrequencyData::AmplitudePart,
                               aocommon::PolarizationEnum::XX, images[0],
                               aocommon::PolarizationEnum::YY, images[1]);
  else if (bandParams.nPolarizations == 1)
    tfData = TimeFrequencyData(TimeFrequencyData::AmplitudePart,
                               aocommon::PolarizationEnum::StokesI, images[0]);
  else
    throw std::runtime_error(
        "Don't know how to convert the polarizations in this data set");
  if (mask) tfData.SetGlobalMask(mask);

  const TimeFrequencyMetaDataPtr metaData(new TimeFrequencyMetaData());

  const H5::CompType obsParamsType(sizeof(ObsParams));
  obsParamsType.insertMember("MJD", HOFFSET(ObsParams, mjd),
                             H5::PredType::NATIVE_DOUBLE);
  const H5::DataSet obsParamsSet =
      file.openDataSet(beamPrefix + "/metadata/obs_params");
  const H5::DataSpace obsParamsSpace = obsParamsSet.getSpace();
  if (obsParamsSpace.getSimpleExtentNdims() != 1)
    throw std::runtime_error(
        "Incorrect dimensionality of metadata/obs_params for this beam/band");
  hsize_t obsParamsDims;
  obsParamsSpace.getSimpleExtentDims(&obsParamsDims);
  if (obsParamsDims != nTimes)
    throw std::runtime_error(
        "Incorrect nr of elements in metadata/obs_params data set for this "
        "beam/band");
  std::vector<ObsParams> obsParams(nTimes);
  obsParamsSet.read(obsParams.data(), obsParamsType);
  std::vector<double> times;
  times.reserve(nTimes);
  for (const ObsParams& obsParamsEl : obsParams)
    times.emplace_back(obsParamsEl.mjd * 24.0 * 60.0 * 60.0);
  metaData->SetObservationTimes(std::move(times));

  const H5::DataSet frequencySet =
      file.openDataSet(beamPrefix + "/astronomy_data/frequency");
  const H5::DataSpace frequencySpace = frequencySet.getSpace();
  if (frequencySpace.getSimpleExtentNdims() != 1)
    throw std::runtime_error(
        "Incorrect dimensionality of astronomy_data/frequency for this "
        "beam/band");
  hsize_t frequencySpaceDims;
  frequencySpace.getSimpleExtentDims(&frequencySpaceDims);
  if (int(frequencySpaceDims) != bandParams.nChannels)
    throw std::runtime_error(
        "Incorrect nr of elements (" + std::to_string(frequencySpaceDims) +
        ") in astronomy_data/frequency for this beam/band");
  std::vector<float> channels(bandParams.nChannels);
  frequencySet.read(channels.data(), H5::PredType::NATIVE_FLOAT);
  BandInfo bandInfo;
  bandInfo.channels.resize(bandParams.nChannels);
  for (int ch = 0; ch != bandParams.nChannels; ++ch) {
    bandInfo.channels[ch].frequencyHz = channels[ch] * 1e6;
    bandInfo.channels[ch].frequencyIndex = ch;
  }
  metaData->SetBand(bandInfo);

  return BaselineData(tfData, metaData, index);
}

void SdhdfImageSet::PerformReadRequests(ProgressListener& progress) {
  progress.OnStartTask("Reading " + _path);
  for (size_t i = 0; i != _requests.size(); ++i) {
    const ImageSetIndex& index = _requests[i];
    SubTaskListener subListener(progress, i, _requests.size());
    _baselineBuffer.emplace(loadData(subListener, index));
    progress.OnProgress(i + 1, _requests.size());
  }
  progress.OnFinish();
}

void SdhdfImageSet::AddWriteFlagsTask(const ImageSetIndex& index,
                                      std::vector<Mask2DCPtr>& flags) {
  assert(!flags.empty());
  const size_t beam = _indexTable[index.Value()].first;
  const size_t band = _indexTable[index.Value()].second;
  const BandParams& bandParams = _beams[beam].bandParams[band];
  const std::string label = bandParams.label;
  const size_t nTimes = flags.front()->Width();
  Logger::Info << "Writing flags for beam " << beam << ", " << label << "...\n";
  H5::H5File file(_path, H5F_ACC_RDWR);

  const std::string beamPrefix =
      "beam_" + std::to_string(beam) + '/' + label + '/';
  const std::string dataPrefix = beamPrefix + "astronomy_data/";

  H5::DataSet flagsSet;
  if (!tryOpen(flagsSet, file, dataPrefix + "flag")) {
    // Create the data set
    const H5::DataType dataType(H5::PredType::NATIVE_UINT8);
    const hsize_t dimsf[2]{nTimes, hsize_t(bandParams.nChannels)};
    const H5::DataSpace dataSpace(2, dimsf);
    flagsSet = file.createDataSet(dataPrefix + "flag", dataType, dataSpace);
  }

  Logger::Info << "Writing flag data...\n";
  aocommon::UVector<unsigned char> flagData(nTimes * bandParams.nChannels);
  unsigned char* valuePtr = flagData.data();
  for (size_t timeIndex = 0; timeIndex != nTimes; ++timeIndex) {
    for (int channel = 0; channel != bandParams.nChannels; ++channel) {
      bool flag = false;
      for (const Mask2DCPtr& mask : flags)
        flag = flag || mask->Value(timeIndex, channel);
      *valuePtr = flag ? 1 : 0;
      ++valuePtr;
    }
  }
  flagsSet.write(flagData.data(), H5::PredType::NATIVE_UINT8);
}

}  // namespace imagesets
