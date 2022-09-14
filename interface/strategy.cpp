#include "aoflagger.h"
#include "structures.h"

#include "../lua/luastrategy.h"
#include "../lua/scriptdata.h"

#include "../util/progress/progresslistener.h"

#include "../structures/timefrequencydata.h"

#include <mutex>

namespace aoflagger {

class ErrorListener : public ProgressListener {
  virtual void OnStartTask(const std::string&) final override {}
  virtual void OnProgress(size_t, size_t) final override {}
  virtual void OnFinish() final override {}
  virtual void OnException(std::exception& e) final override {
    std::cerr << "*** EXCEPTION OCCURED IN THE AOFLAGGER ***\n"
                 "The AOFlagger encountered a bug or the given strategy was "
                 "invalid!\n"
                 "The reported exception "
              << typeid(e).name() << " is:\n"
              << e.what();
  }
};

class ForwardingListener : public ProgressListener {
 public:
  explicit ForwardingListener(StatusListener* destination)
      : _destination(destination) {}
  virtual void OnStartTask(const std::string& description) final override {
    _destination->OnStartTask(description);
  }
  virtual void OnProgress(size_t progress, size_t maxProgress) final override {
    _destination->OnProgress(progress, maxProgress);
  }
  virtual void OnFinish() final override { _destination->OnFinish(); }
  virtual void OnException(std::exception& thrownException) final override {
    _destination->OnException(thrownException);
  }

 private:
  StatusListener* _destination;
};

class StrategyData {
 public:
  [[no_unique_address]] LuaStrategy _lua;
};

Strategy::Strategy() : _data(), _aoflagger(nullptr) {}

Strategy::Strategy(const std::string& filename, AOFlagger* aoflagger)
    : _data(new StrategyData()), _aoflagger(aoflagger) {
  _data->_lua.Initialize();
  _data->_lua.LoadFile(filename.c_str());
}

Strategy::Strategy(Strategy&& sourceStrategy)
    : _data(sourceStrategy._data
                ? new StrategyData(std::move(*sourceStrategy._data))
                : nullptr),
      _aoflagger(sourceStrategy._aoflagger) {}

Strategy::~Strategy() {}

Strategy Strategy::makeFromString(const std::string& script,
                                  AOFlagger* aoflagger) {
  Strategy strategy;
  strategy._data.reset(new StrategyData());
  strategy._aoflagger = aoflagger;
  strategy._data->_lua.Initialize();
  strategy._data->_lua.LoadText(script);
  return strategy;
}

Strategy& Strategy::operator=(Strategy&& sourceStrategy) {
  if (sourceStrategy._data)
    _data = std::move(sourceStrategy._data);
  else
    _data = nullptr;
  _aoflagger = sourceStrategy._aoflagger;
  return *this;
}

inline static AntennaInfo ConvertAntenna(Antenna& antenna) {
  AntennaInfo antennaInfo;
  antennaInfo.diameter = antenna.diameter;
  antennaInfo.id = antenna.id;
  antennaInfo.mount = antenna.mount;
  antennaInfo.name = antenna.name;
  antennaInfo.station = antenna.station;
  antennaInfo.position.x = antenna.x;
  antennaInfo.position.y = antenna.y;
  antennaInfo.position.z = antenna.z;
  return antennaInfo;
}

inline static BandInfo ConvertBand(Band& band) {
  BandInfo bandInfo;
  bandInfo.windowIndex = band.id;
  bandInfo.channels.resize(band.channels.size());
  for (size_t i = 0; i != band.channels.size(); ++i) {
    bandInfo.channels[i].frequencyIndex = i;
    bandInfo.channels[i].frequencyHz = band.channels[i].frequency;
    bandInfo.channels[i].channelWidthHz = band.channels[i].width;
    bandInfo.channels[i].effectiveBandWidthHz = band.channels[i].width;
    bandInfo.channels[i].resolutionHz = band.channels[i].width;
  }
  return bandInfo;
}

FlagMask Strategy::Run(const ImageSet& input,
                       const FlagMask& preExistingFlags) {
  return run(input, &preExistingFlags);
}

FlagMask Strategy::Run(const ImageSet& input) { return run(input, nullptr); }

FlagMask Strategy::run(const ImageSet& input,
                       const FlagMask* preExistingFlags) {
  std::unique_ptr<ProgressListener> listener;
  if (_aoflagger->_statusListener == nullptr)
    listener.reset(new ErrorListener());
  else
    listener.reset(new ForwardingListener(_aoflagger->_statusListener));

  Mask2DCPtr inputMask;
  if (preExistingFlags == nullptr)
    inputMask = Mask2D::CreateSetMaskPtr<false>(input.Width(), input.Height());
  else
    inputMask = preExistingFlags->_data->mask;

  TimeFrequencyData inputData;
  Image2DPtr zeroImage =
      Image2D::CreateZeroImagePtr(input.Width(), input.Height());
  switch (input.ImageCount()) {
    case 1:
      inputData = TimeFrequencyData(TimeFrequencyData::AmplitudePart,
                                    aocommon::Polarization::StokesI,
                                    input._data->images[0]);
      inputData.SetGlobalMask(inputMask);
      break;
    case 2:
      inputData =
          TimeFrequencyData(aocommon::Polarization::StokesI,
                            input._data->images[0], input._data->images[1]);
      inputData.SetGlobalMask(inputMask);
      break;
    case 4:
      inputData =
          TimeFrequencyData(aocommon::Polarization::XX, input._data->images[0],
                            input._data->images[1], aocommon::Polarization::YY,
                            input._data->images[2], input._data->images[3]);
      inputData.SetIndividualPolarizationMasks(inputMask, inputMask);
      break;
    case 8:
      inputData = TimeFrequencyData::FromLinear(
          input._data->images[0], input._data->images[1],
          input._data->images[2], input._data->images[3],
          input._data->images[4], input._data->images[5],
          input._data->images[6], input._data->images[7]);
      inputData.SetIndividualPolarizationMasks(inputMask, inputMask, inputMask,
                                               inputMask);
      break;
  }

  TimeFrequencyMetaDataPtr metaData(new TimeFrequencyMetaData());
  if (input.HasAntennas() && !_aoflagger->_antennas.empty()) {
    metaData->SetAntenna1(
        ConvertAntenna(_aoflagger->_antennas[input.Antenna1()]));
    metaData->SetAntenna2(
        ConvertAntenna(_aoflagger->_antennas[input.Antenna2()]));
  }
  if (input.HasBand() && !_aoflagger->_bands.empty()) {
    metaData->SetBand(ConvertBand(_aoflagger->_bands[input.Band()]));
  }
  if (input.HasInterval() && !_aoflagger->_intervals.empty()) {
    metaData->SetObservationTimes(
        _aoflagger->_intervals[input.Interval()].times);
  }
  ScriptData scriptData;
  scriptData.SetProgressListener(*listener);

  _data->_lua.Execute(inputData, metaData, scriptData, "execute");

  listener.reset();
  inputMask.reset();

  FlagMask flagMask;
  flagMask._data.reset(
      new FlagMaskData(Mask2DPtr(new Mask2D(*inputData.GetSingleMask()))));
  return flagMask;
}

}  // namespace aoflagger
