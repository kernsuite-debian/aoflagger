#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

#include "msmetadata.h"
#include "date.h"

#include "../util/logger.h"

MSMetaData::~MSMetaData() {}

size_t MSMetaData::BandCount(const std::string &location) {
  casacore::MeasurementSet ms(location);
  return ms.spectralWindow().nrow();
}

void MSMetaData::initializeOtherData() {
  casacore::MeasurementSet ms(_path);
  initializeAntennas(ms);
  initializeBands(ms);
  initializeFields(ms);
}

void MSMetaData::initializeAntennas(casacore::MeasurementSet &ms) {
  casacore::MSAntenna antennaTable = ms.antenna();
  size_t count = antennaTable.nrow();
  casacore::ArrayColumn<double> positionCol(antennaTable, "POSITION");
  casacore::ScalarColumn<casacore::String> nameCol(antennaTable, "NAME");
  casacore::ScalarColumn<double> diameterCol(antennaTable, "DISH_DIAMETER");
  casacore::ScalarColumn<casacore::String> mountCol(antennaTable, "MOUNT");
  casacore::ScalarColumn<casacore::String> stationCol(antennaTable, "STATION");

  _antennas.resize(count);
  for (size_t row = 0; row != count; ++row) {
    AntennaInfo info;
    info.diameter = diameterCol(row);
    info.id = row;
    info.name = nameCol(row);
    casacore::Array<double> position = positionCol(row);
    casacore::Array<double>::iterator i = position.begin();
    info.position.x = *i;
    ++i;
    info.position.y = *i;
    ++i;
    info.position.z = *i;
    info.mount = mountCol(row);
    info.station = stationCol(row);
    _antennas[row] = info;
  }
}

void MSMetaData::initializeBands(casacore::MeasurementSet &ms) {
  casacore::MSSpectralWindow spectralWindowTable = ms.spectralWindow();
  casacore::ScalarColumn<int> numChanCol(spectralWindowTable, "NUM_CHAN");
  casacore::ArrayColumn<double> frequencyCol(spectralWindowTable, "CHAN_FREQ");

  _bands.resize(spectralWindowTable.nrow());
  for (size_t bandIndex = 0; bandIndex != spectralWindowTable.nrow();
       ++bandIndex) {
    BandInfo band;
    band.windowIndex = bandIndex;
    size_t channelCount = numChanCol(bandIndex);

    const casacore::Array<double> &frequencies = frequencyCol(bandIndex);
    casacore::Array<double>::const_iterator frequencyIterator =
        frequencies.begin();

    for (unsigned channel = 0; channel < channelCount; ++channel) {
      ChannelInfo channelInfo;
      channelInfo.frequencyIndex = channel;
      channelInfo.frequencyHz = frequencies(casacore::IPosition(1, channel));
      channelInfo.channelWidthHz = 0.0;
      channelInfo.effectiveBandWidthHz = 0.0;
      channelInfo.resolutionHz = 0.0;
      band.channels.push_back(channelInfo);

      ++frequencyIterator;
    }

    _bands[bandIndex] = band;
  }
}

void MSMetaData::initializeFields(casacore::MeasurementSet &ms) {
  casacore::MSField fieldTable = ms.field();
  casacore::ArrayColumn<double> delayDirectionCol(
      fieldTable, fieldTable.columnName(casacore::MSFieldEnums::DELAY_DIR));
  casacore::ScalarColumn<casacore::String> nameCol(
      fieldTable, fieldTable.columnName(casacore::MSFieldEnums::NAME));

  _fields.resize(fieldTable.nrow());
  for (size_t row = 0; row != fieldTable.nrow(); ++row) {
    const casacore::Array<double> &delayDirection = delayDirectionCol(row);
    casacore::Array<double>::const_iterator delayDirectionIterator =
        delayDirection.begin();

    FieldInfo field;
    field.fieldId = row;
    field.delayDirectionRA = *delayDirectionIterator;
    ++delayDirectionIterator;
    field.delayDirectionDec = *delayDirectionIterator;
    field.name = nameCol(row);

    _fields[row] = field;
  }
}

std::string MSMetaData::GetTelescopeName(casacore::MeasurementSet &ms) {
  casacore::MSObservation obsTable = ms.observation();
  casacore::ScalarColumn<casacore::String> telescopeNameCol(
      obsTable,
      obsTable.columnName(casacore::MSObservationEnums::TELESCOPE_NAME));
  if (obsTable.nrow() != 0) {
    std::string telescopeName = telescopeNameCol(0);
    for (size_t row = 1; row != obsTable.nrow(); ++row) {
      if (std::string(telescopeNameCol(row)) != telescopeName)
        throw std::runtime_error(
            "The OBSERVATION table contains multiple entries from different "
            "telescopes. I do not know how to handle such sets.");
    }
    return telescopeName;
  }
  throw std::runtime_error("Measurement set contains no observations");
}

void MSMetaData::GetDataDescToBandVector(std::vector<size_t> &dataDescToBand) {
  casacore::MeasurementSet ms(_path);
  casacore::MSDataDescription dataDescTable = ms.dataDescription();
  casacore::ScalarColumn<int> spwIdCol(
      dataDescTable, dataDescTable.columnName(
                         casacore::MSDataDescriptionEnums::SPECTRAL_WINDOW_ID));
  dataDescToBand.resize(dataDescTable.nrow());
  for (size_t dataDescId = 0; dataDescId != dataDescTable.nrow();
       ++dataDescId) {
    dataDescToBand[dataDescId] = spwIdCol(dataDescId);
  }
}

void MSMetaData::initializeMainTableData() {
  if (!_isMainTableDataInitialized) {
    Logger::Debug << "Initializing ms metadata cache data...\n";

    casacore::MeasurementSet ms(_path);
    casacore::ScalarColumn<int> antenna1Col(
        ms, casacore::MeasurementSet::columnName(
                casacore::MeasurementSet::ANTENNA1));
    casacore::ScalarColumn<int> antenna2Col(
        ms, casacore::MeasurementSet::columnName(
                casacore::MeasurementSet::ANTENNA2));
    casacore::ScalarColumn<int> fieldIdCol(
        ms, casacore::MeasurementSet::columnName(
                casacore::MeasurementSet::FIELD_ID));
    casacore::ScalarColumn<int> dataDescIdCol(
        ms, casacore::MeasurementSet::columnName(
                casacore::MeasurementSet::DATA_DESC_ID));
    casacore::ScalarColumn<double> timeCol(
        ms,
        casacore::MeasurementSet::columnName(casacore::MeasurementSet::TIME));

    double time = -1.0;
    std::set<std::pair<size_t, size_t> > baselineSet;
    std::set<Sequence> sequenceSet;
    size_t prevFieldId = size_t(-1), sequenceId = size_t(-1);
    for (size_t row = 0; row != ms.nrow(); ++row) {
      size_t a1 = antenna1Col(row), a2 = antenna2Col(row),
             fieldId = fieldIdCol(row), spw = dataDescIdCol(row);
      double cur_time = timeCol(row);

      bool isNewTime = cur_time != time;
      if (fieldId != prevFieldId) {
        prevFieldId = fieldId;
        sequenceId++;
        _observationTimesPerSequence.emplace_back();
      }
      if (isNewTime) {
        time = cur_time;
        _observationTimesPerSequence[sequenceId].insert(cur_time);
        _observationTimes.emplace_hint(_observationTimes.end(), cur_time);
      }

      baselineSet.insert(std::pair<size_t, size_t>(a1, a2));
      sequenceSet.insert(Sequence(a1, a2, spw, sequenceId, fieldId));
    }

    _baselines.assign(baselineSet.begin(), baselineSet.end());
    _sequences.assign(sequenceSet.begin(), sequenceSet.end());

    if (_intervalEnd) {
      for (std::set<double> &seq : _observationTimesPerSequence) {
        if (seq.size() > _intervalEnd.get())
          seq.erase(std::next(seq.begin(), _intervalEnd.get()), seq.end());
      }
      _observationTimes.erase(
          std::next(_observationTimes.begin(), _intervalEnd.get()),
          _observationTimes.end());
    }
    if (_intervalStart) {
      for (std::set<double> &seq : _observationTimesPerSequence) {
        if (seq.size() > _intervalStart.get())
          seq.erase(seq.begin(), std::next(seq.begin(), _intervalStart.get()));
        else
          seq.clear();
      }
      _observationTimes.erase(
          _observationTimes.begin(),
          std::next(_observationTimes.begin(), _intervalStart.get()));
    }
    _isMainTableDataInitialized = true;
  }
}

size_t MSMetaData::PolarizationCount(const std::string &filename) {
  casacore::MeasurementSet ms(filename);
  casacore::Table polTable = ms.polarization();
  casacore::ArrayColumn<int> corTypeColumn(polTable, "CORR_TYPE");
  casacore::Array<int> corType = corTypeColumn(0);
  casacore::Array<int>::iterator iterend(corType.end());
  size_t polarizationCount = 0;
  for (casacore::Array<int>::iterator iter = corType.begin(); iter != iterend;
       ++iter) {
    ++polarizationCount;
  }
  return polarizationCount;
}

bool MSMetaData::HasAOFlaggerHistory() {
  casacore::MeasurementSet ms(_path);
  casacore::Table histtab(ms.history());
  casacore::ScalarColumn<casacore::String> application(histtab, "APPLICATION");
  for (unsigned i = 0; i < histtab.nrow(); ++i) {
    if (application(i) == "AOFlagger") return true;
  }
  return false;
}

void MSMetaData::GetAOFlaggerHistory(std::ostream &stream) {
  casacore::MeasurementSet ms(_path);
  casacore::MSHistory histtab(ms.history());
  casacore::ScalarColumn<double> time(histtab, "TIME");
  casacore::ScalarColumn<casacore::String> application(histtab, "APPLICATION");
  casacore::ArrayColumn<casacore::String> cli(histtab, "CLI_COMMAND");
  casacore::ArrayColumn<casacore::String> parms(histtab, "APP_PARAMS");
  for (unsigned i = 0; i < histtab.nrow(); ++i) {
    if (application(i) == "AOFlagger") {
      stream << "====================\n"
                "Command: "
             << *cli(i).begin()
             << "\n"
                "Date: "
             << Date::AipsMJDToDateString(time(i))
             << "\n"
                "Time: "
             << Date::AipsMJDToTimeString(time(i))
             << "\n"
                "Strategy: \n     ----------     \n";
      const casacore::Vector<casacore::String> appParamsVec = parms(i);
      for (casacore::Vector<casacore::String>::const_iterator j =
               appParamsVec.begin();
           j != appParamsVec.end(); ++j) {
        stream << *j << '\n';
      }
      stream << "     ----------     \n";
    }
  }
}

void MSMetaData::AddAOFlaggerHistory(const std::string &strategy,
                                     const std::string &commandline) {
  // This has been copied from MSWriter.cc of NDPPP and altered (thanks, Ger!)
  casacore::MeasurementSet ms(_path);
  casacore::Table histtab(ms.history());
  histtab.reopenRW();
  casacore::ScalarColumn<double> time(histtab, "TIME");
  casacore::ScalarColumn<int> obsId(histtab, "OBSERVATION_ID");
  casacore::ScalarColumn<casacore::String> message(histtab, "MESSAGE");
  casacore::ScalarColumn<casacore::String> application(histtab, "APPLICATION");
  casacore::ScalarColumn<casacore::String> priority(histtab, "PRIORITY");
  casacore::ScalarColumn<casacore::String> origin(histtab, "ORIGIN");
  casacore::ArrayColumn<casacore::String> parms(histtab, "APP_PARAMS");
  casacore::ArrayColumn<casacore::String> cli(histtab, "CLI_COMMAND");
  // Put all parset entries in a Vector<String>.
  // Some WSRT MSs have a FixedShape APP_PARAMS and CLI_COMMAND column.
  // For them, put the xml file in a single vector element (with newlines).
  bool fixedShaped =
      (parms.columnDesc().options() & casacore::ColumnDesc::FixedShape) != 0;

  casacore::Vector<casacore::String> appParamsVec;
  casacore::Vector<casacore::String> clivec;
  clivec.resize(1);
  clivec[0] = commandline;
  if (fixedShaped) {
    appParamsVec.resize(1);
    appParamsVec[0] = strategy;
  } else {
    // Tokenize the string on '\n'
    const std::string str = strategy;
    size_t lineCount = std::count(str.begin(), str.end(), '\n');
    appParamsVec.resize(lineCount + 1);
    casacore::Array<casacore::String>::contiter viter = appParamsVec.cbegin();
    size_t curStringPos = 0;
    for (size_t i = 0; i < lineCount; ++i) {
      size_t endPos = str.find('\n', curStringPos);
      *viter = str.substr(curStringPos, endPos - curStringPos);
      ++viter;
      curStringPos = endPos + 1;
    }
    if (curStringPos < str.size()) {
      *viter = str.substr(curStringPos, str.size() - curStringPos);
    }
  }
  uint rownr = histtab.nrow();
  histtab.addRow();
  time.put(rownr, casacore::Time().modifiedJulianDay() * 24.0 * 3600.0);
  obsId.put(rownr, 0);
  message.put(rownr, "parameters");
  application.put(rownr, "AOFlagger");
  priority.put(rownr, "NORMAL");
  origin.put(rownr, "standalone");
  parms.put(rownr, appParamsVec);
  cli.put(rownr, clivec);
}

std::string MSMetaData::GetStationName() const {
  casacore::MeasurementSet ms(_path);
  casacore::Table antennaTable(ms.antenna());
  if (antennaTable.nrow() == 0)
    throw std::runtime_error("GetStationName() : no rows in Antenna table");
  casacore::ScalarColumn<casacore::String> stationColumn(antennaTable,
                                                         "STATION");
  return stationColumn(0);
}

bool MSMetaData::IsChannelZeroRubish() {
  try {
    const std::string station = GetStationName();
    if (station != "LOFAR") return false;
    // This is of course a hack, but its the best estimate we can make :-/
    // (easily)
    const BandInfo bandInfo = GetBandInfo(0);
    return (bandInfo.channels.size() == 256 || bandInfo.channels.size() == 64);
  } catch (std::exception &e) {
    return false;
  }
}
