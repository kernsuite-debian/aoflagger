#include "fitsimageset.h"

#include <sstream>
#include <vector>

#include "../structures/date.h"
#include "../msio/fitsfile.h"
#include "../structures/image2d.h"
#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

#include "../util/logger.h"

namespace imagesets {

FitsImageSet::FitsImageSet(const std::string& file)
    : ImageSet(),
      _file(new FitsFile(file)),
      _currentBaselineIndex(0),
      _frequencyOffset(0.0),
      _fitsType(UVFitsType) {
  _file->Open(FitsFile::ReadWriteMode);
}

FitsImageSet::FitsImageSet(const FitsImageSet& source)
    : ImageSet(),
      _file(source._file),
      _baselines(source._baselines),
      _bandCount(source._bandCount),
      _antennaInfos(source._antennaInfos),
      _bandInfos(source._bandInfos),
      _bandIndexToNumber(source._bandIndexToNumber),
      _currentBaselineIndex(source._currentBaselineIndex),
      _currentBandIndex(source._currentBandIndex),
      _frequencyOffset(source._frequencyOffset),
      _baselineData(source._baselineData),
      _fitsType(source._fitsType) {}

FitsImageSet::~FitsImageSet() {}

std::unique_ptr<ImageSet> FitsImageSet::Clone() {
  return std::unique_ptr<FitsImageSet>(new FitsImageSet(*this));
}

void FitsImageSet::Initialize() {
  if (_file->HasGroups())
    _fitsType = UVFitsType;
  else if (_file->GetHDUCount() > 1)
    _fitsType = SDFitsType;
  else
    _fitsType = DynSpectrumType;

  switch (_fitsType) {
    case UVFitsType: {
      Logger::Debug << "This file has " << _file->GetGroupCount()
                    << " groups with " << _file->GetParameterCount()
                    << " parameters.\n";
      _file->MoveToHDU(1);
      if (_file->GetCurrentHDUType() != FitsFile::ImageHDUType)
        throw FitsIOException("Primary table is not a grouped image");
      std::vector<long double> parameters(_file->GetParameterCount());
      const int baselineIndex = _file->GetGroupParameterIndex("BASELINE");
      const size_t groupCount = _file->GetGroupCount();
      std::set<std::pair<size_t, size_t>> baselineSet;
      for (size_t g = 0; g < groupCount; ++g) {
        _file->ReadGroupParameters(g, parameters.data());
        const int a1 = (((int)parameters[baselineIndex]) & 255) - 1;
        const int a2 = (((int)parameters[baselineIndex] >> 8) & 255) - 1;
        baselineSet.insert(std::pair<size_t, size_t>(a1, a2));
      }
      Logger::Debug << "Baselines in file: " << baselineSet.size() << '\n';
      for (std::set<std::pair<size_t, size_t>>::const_iterator i =
               baselineSet.begin();
           i != baselineSet.end(); ++i)
        _baselines.push_back(*i);
      _bandCount = _file->GetCurrentImageSize(5);
    } break;

    case SDFitsType: {
      _baselines.push_back(std::pair<size_t, size_t>(0, 0));
      AntennaInfo antenna;
      antenna.id = 0;
      _antennaInfos.push_back(antenna);

      // find number of bands
      _file->MoveToHDU(2);
      int ifColumn = 0;
      const bool hasIF = _file->HasTableColumn("IF", ifColumn);
      if (!hasIF) ifColumn = _file->GetTableColumnIndex("IFNUM");
      const int rowCount = _file->GetRowCount();
      std::set<int> ifSet;
      for (int i = 1; i <= rowCount; ++i) {
        double thisIndex;
        _file->ReadTableCell(i, ifColumn, &thisIndex, 1);
        ifSet.insert((int)round(thisIndex));
      }
      _bandCount = ifSet.size();
      if (_bandCount == 0)
        throw std::runtime_error("Could not find any IF's in this set");
      _bandIndexToNumber.clear();
      Logger::Debug << _bandCount << " IF's in set: [" << *ifSet.begin();
      for (const int i : ifSet) {
        _bandInfos.emplace(i, BandInfo());
        if (_bandIndexToNumber.size() > 0) Logger::Debug << ", " << i;
        _bandIndexToNumber.push_back(i);
      }
      Logger::Debug << "]\n";
    } break;

    case DynSpectrumType:
      _baselines.push_back(std::pair<size_t, size_t>(0, 0));
      AntennaInfo antenna;
      antenna.id = 0;
      antenna.name = "";
      const size_t height = _file->GetCurrentImageSize(2);
      _antennaInfos.emplace_back(antenna);
      _bandCount = 1;
      _bandInfos.emplace(0, BandInfo());
      _bandInfos[0].channels.resize(height);
      const double freq0 = _file->GetDoubleKeywordValue("CRVAL2");
      const double freqDelta = _file->GetDoubleKeywordValue("CDELT2");
      _sourceName = _file->GetKeywordValue("SOURCE");
      for (size_t i = 0; i != _bandInfos[0].channels.size(); ++i) {
        _bandInfos[0].channels[i].frequencyHz = freq0 + i * freqDelta;
        _bandInfos[0].channels[i].frequencyIndex = i;
      }
      _file->MoveToHDU(1);
      break;
  }
}

BaselineData FitsImageSet::loadData(const ImageSetIndex& index) {
  const size_t baselineIndex = index.Value() / _bandCount;
  const size_t bandIndex = index.Value() % _bandCount;
  _frequencyOffset = 0.0;

  _file->MoveToHDU(1);
  const TimeFrequencyMetaDataPtr metaData(new TimeFrequencyMetaData());
  TimeFrequencyData data;
  switch (_fitsType) {
    case UVFitsType:
      data = ReadPrimaryGroupTable(baselineIndex, bandIndex, 0, *metaData);
      break;
    case SDFitsType:
      ReadPrimarySingleTable(data, *metaData);
      break;
    case DynSpectrumType:
      ReadDynSpectrum(data, *metaData);
      return BaselineData(data, metaData, index);
  }

  for (int hduIndex = 2; hduIndex <= _file->GetHDUCount(); hduIndex++) {
    _file->MoveToHDU(hduIndex);
    switch (_file->GetCurrentHDUType()) {
      case FitsFile::BinaryTableHDUType:
        Logger::Debug << "Binary table found.\n";
        ReadTable(data, *metaData, bandIndex);
        break;
      case FitsFile::ASCIITableHDUType:
        Logger::Debug << "ASCII table found.\n";
        ReadTable(data, *metaData, bandIndex);
        break;
      case FitsFile::ImageHDUType:
        Logger::Debug << "Image found.\n";
        break;
    }
  }

  if (_fitsType == UVFitsType) {
    _currentBaselineIndex = baselineIndex;
    _currentBandIndex = bandIndex;
    const int bandNumber = _bandIndexToNumber[bandIndex];
    metaData->SetBand(_bandInfos[bandNumber]);
    metaData->SetAntenna1(
        _antennaInfos[_baselines[_currentBaselineIndex].first]);
    metaData->SetAntenna2(
        _antennaInfos[_baselines[_currentBaselineIndex].second]);
    Logger::Debug << "Loaded metadata for: "
                  << Date::AipsMJDToString(metaData->ObservationTimes()[0])
                  << ", band " << bandNumber << " ("
                  << Frequency::ToString(
                         _bandInfos[bandNumber].channels[0].frequencyHz)
                  << " - "
                  << Frequency::ToString(
                         _bandInfos[bandNumber].channels.rbegin()->frequencyHz)
                  << ")\n";
  } else {
    metaData->SetAntenna1(_antennaInfos[0]);
    metaData->SetAntenna2(_antennaInfos[0]);
  }
  return BaselineData(data, metaData, index);
}

TimeFrequencyData FitsImageSet::ReadPrimaryGroupTable(
    size_t baselineIndex, int band, int stokes,
    TimeFrequencyMetaData& metaData) {
  if (!_file->HasGroups() ||
      _file->GetCurrentHDUType() != FitsFile::ImageHDUType)
    throw FitsIOException("Primary table is not a grouped image");

  std::vector<double> observationTimes;
  std::vector<UVW> uvws;

  std::vector<long double> parameters(_file->GetParameterCount());
  const int baseline = (_baselines[baselineIndex].first + 1) +
                       ((_baselines[baselineIndex].second + 1) << 8);
  const int baselineColumn = _file->GetGroupParameterIndex("BASELINE");
  size_t complexCount = _file->GetCurrentImageSize(2),
         stokesStep = complexCount, stokesCount = _file->GetCurrentImageSize(3),
         frequencyStep = stokesCount * complexCount,
         frequencyCount = _file->GetCurrentImageSize(4),
         bandStep = frequencyStep * frequencyCount;
  std::vector<std::vector<long double>> valuesR(frequencyCount);
  std::vector<std::vector<long double>> valuesI(frequencyCount);
  std::vector<long double> data(_file->GetImageSize());
  const size_t groupCount = _file->GetGroupCount();
  const bool hasDate2 = _file->HasGroupParameter("DATE", 2);
  int date2Index = 0, date1Index = _file->GetGroupParameterIndex("DATE");
  if (hasDate2) {
    date2Index = _file->GetGroupParameterIndex("DATE", 2);
  }
  int uuIndex, vvIndex, wwIndex;
  if (_file->HasGroupParameter("UU")) {
    uuIndex = _file->GetGroupParameterIndex("UU");
    vvIndex = _file->GetGroupParameterIndex("VV");
    wwIndex = _file->GetGroupParameterIndex("WW");
  } else {
    uuIndex = _file->GetGroupParameterIndex("UU---SIN");
    vvIndex = _file->GetGroupParameterIndex("VV---SIN");
    wwIndex = _file->GetGroupParameterIndex("WW---SIN");
  }
  size_t match = 0;
  double frequencyFactor = 1.0;
  if (_frequencyOffset != 0.0) frequencyFactor = _frequencyOffset;
  for (size_t g = 0; g < groupCount; ++g) {
    _file->ReadGroupParameters(g, &parameters[0]);
    if (parameters[baselineColumn] == baseline) {
      double date;
      if (hasDate2)
        date = parameters[date1Index] + parameters[date2Index];
      else
        date = parameters[date1Index];
      UVW uvw;
      uvw.u = parameters[uuIndex] * frequencyFactor;
      uvw.v = parameters[vvIndex] * frequencyFactor;
      uvw.w = parameters[wwIndex] * frequencyFactor;

      _file->ReadGroupData(g, &data[0]);
      for (size_t f = 0; f < frequencyCount; ++f) {
        const size_t index =
            stokes * stokesStep + frequencyStep * f + bandStep * band;
        const long double r = data[index];
        const long double i = data[index + 1];
        valuesR[f].push_back(r);
        valuesI[f].push_back(i);
      }
      observationTimes.push_back(Date::JDToAipsMJD(date));
      uvws.push_back(uvw);
      ++match;
    }
  }
  Logger::Debug << match << " rows in table matched baseline.\n";
  data.clear();
  parameters.clear();

  Logger::Debug << "Image is " << valuesR[0].size() << " x " << frequencyCount
                << '\n';
  if (valuesR[0].size() == 0) throw std::runtime_error("Baseline not found!");
  Image2DPtr real = Image2D::CreateUnsetImagePtr(valuesR[0].size(),
                                                 frequencyCount),
             imaginary = Image2D::CreateUnsetImagePtr(valuesR[0].size(),
                                                      frequencyCount);
  for (size_t i = 0; i < valuesR[0].size(); ++i) {
    for (size_t f = 0; f < frequencyCount; ++f) {
      real->SetValue(i, f, valuesR[f][i]);
      imaginary->SetValue(i, f, valuesI[f][i]);
    }
  }

  metaData.SetUVW(uvws);
  metaData.SetObservationTimes(observationTimes);
  return TimeFrequencyData(aocommon::Polarization::StokesI, real, imaginary);
}

void FitsImageSet::ReadPrimarySingleTable(TimeFrequencyData& data,
                                          TimeFrequencyMetaData& metaData) {}

void FitsImageSet::ReadTable(TimeFrequencyData& data,
                             TimeFrequencyMetaData& metaData,
                             size_t bandIndex) {
  const std::string extName = _file->GetKeywordValue("EXTNAME");
  if (extName == "AIPS AN")
    ReadAntennaTable(metaData);
  else if (extName == "AIPS FQ")
    ReadFrequencyTable(data, metaData);
  else if (extName == "AIPS CL")
    ReadCalibrationTable();
  else if (extName == "SINGLE DISH")
    ReadSingleDishTable(data, metaData, bandIndex);
}

void FitsImageSet::ReadAntennaTable(TimeFrequencyMetaData& metaData) {
  Logger::Debug << "Found antenna table\n";
  _frequencyOffset = _file->GetDoubleKeywordValue("FREQ");
  for (std::map<int, BandInfo>::iterator i = _bandInfos.begin();
       i != _bandInfos.end(); ++i) {
    for (std::vector<ChannelInfo>::iterator j = i->second.channels.begin();
         j != i->second.channels.end(); ++j) {
      j->frequencyHz += _frequencyOffset;
    }
  }
  std::vector<UVW> uvws(metaData.UVW());
  for (std::vector<UVW>::iterator i = uvws.begin(); i != uvws.end(); ++i) {
    i->u = i->u * _frequencyOffset;
    i->v = i->v * _frequencyOffset;
    i->w = i->w * _frequencyOffset;
  }
  metaData.SetUVW(uvws);
  _antennaInfos.clear();
  for (int i = 1; i <= _file->GetRowCount(); ++i) {
    AntennaInfo info;
    char name[9];
    long double pos[3];
    _file->ReadTableCell(i, 1, name);
    _file->ReadTableCell(i, 2, pos, 3);
    info.name = name;
    info.id = _antennaInfos.size();
    info.position.x = pos[0];
    info.position.y = pos[1];
    info.position.z = pos[2];
    _antennaInfos.push_back(info);
  }
}

void FitsImageSet::ReadFrequencyTable(TimeFrequencyData& data,
                                      TimeFrequencyMetaData& metaData) {
  Logger::Debug << "Found frequency table\n";
  const size_t numberIfs = _file->GetIntKeywordValue("NO_IF");
  Logger::Debug << "Number of ifs: " << numberIfs << '\n';
  _bandInfos.clear();
  BandInfo bandInfo;
  for (int i = 1; i <= _file->GetRowCount(); ++i) {
    long double freqSel;
    std::vector<long double> ifFreq(numberIfs), chWidth(numberIfs),
        totalBandwidth(numberIfs), sideband(numberIfs);
    _file->ReadTableCell(i, 1, &freqSel, 1);
    _file->ReadTableCell(i, 2, &ifFreq[0], numberIfs);
    _file->ReadTableCell(i, 3, &chWidth[0], numberIfs);
    _file->ReadTableCell(i, 4, &totalBandwidth[0], numberIfs);
    _file->ReadTableCell(i, 5, &sideband[0], numberIfs);
    for (size_t b = 0; b < numberIfs; ++b) {
      for (size_t channel = 0; channel < data.ImageHeight(); ++channel) {
        ChannelInfo channelInfo;
        channelInfo.channelWidthHz = chWidth[b];
        channelInfo.effectiveBandWidthHz = chWidth[b];
        channelInfo.frequencyHz =
            _frequencyOffset + ifFreq[b] + (chWidth[b] * channel);
        channelInfo.frequencyIndex = channel;
        channelInfo.resolutionHz = chWidth[b];
        bandInfo.channels.push_back(channelInfo);
      }

      bandInfo.windowIndex = b;
      _bandInfos.insert(std::pair<int, BandInfo>(b, bandInfo));
    }
  }
}

void FitsImageSet::ReadCalibrationTable() {
  Logger::Debug << "Found calibration table with " << _file->GetRowCount()
                << " rows.\n";
}

void FitsImageSet::ReadDynSpectrum(TimeFrequencyData& data,
                                   TimeFrequencyMetaData& metaData) {
  _file->MoveToHDU(1);
  const size_t width = _file->GetCurrentImageSize(1);
  const size_t height = _file->GetCurrentImageSize(2);
  const size_t npol = _file->GetCurrentImageSize(3);
  Logger::Debug << "Reading fits file with dynspectrum, " << width << " x "
                << height << " x " << npol << "\n";
  if (npol != 4)
    throw std::runtime_error(
        "Expected four polarizations in dynamic spectrum fits file");
  const size_t n = width * height * npol;
  std::vector<num_t> buffer(n);
  _file->ReadCurrentImageData(0, buffer.data(), n);
  Image2DPtr imgs[4];
  for (size_t i = 0; i != 4; ++i)
    imgs[i] = Image2D::CreateUnsetImagePtr(width, height);
  const Mask2DPtr flags = Mask2D::CreateSetMask<false>(width, height);
  std::vector<num_t>::const_iterator bufferIter = buffer.begin();
  for (size_t j = 0; j != npol; ++j) {
    for (size_t y = 0; y != height; ++y) {
      for (size_t x = 0; x != width; ++x) {
        imgs[j]->SetValue(x, y, *bufferIter);
        if (!std::isfinite(*bufferIter)) flags->SetValue(x, y, true);
        ++bufferIter;
      }
    }
  }
  data = TimeFrequencyData::MakeFromPolarizationCombination(
      TimeFrequencyData(TimeFrequencyData::RealPart,
                        aocommon::Polarization::StokesI, imgs[0]),
      TimeFrequencyData(TimeFrequencyData::RealPart,
                        aocommon::Polarization::StokesQ, imgs[1]),
      TimeFrequencyData(TimeFrequencyData::RealPart,
                        aocommon::Polarization::StokesU, imgs[2]),
      TimeFrequencyData(TimeFrequencyData::RealPart,
                        aocommon::Polarization::StokesV, imgs[3]));
  data.SetGlobalMask(flags);
  metaData.SetBand(_bandInfos[0]);
  metaData.SetAntenna1(_antennaInfos[0]);
  metaData.SetAntenna2(_antennaInfos[0]);
  std::vector<double> times(width);
  const double timeDelta = _file->GetDoubleKeywordValue("CDELT1");
  for (size_t i = 0; i != width; ++i) times[i] = timeDelta * i;
  metaData.SetObservationTimes(times);
}

void FitsImageSet::ReadSingleDishTable(TimeFrequencyData& data,
                                       TimeFrequencyMetaData& metaData,
                                       size_t ifIndex) {
  const int rowCount = _file->GetRowCount();
  Logger::Debug << "Found single dish table with " << rowCount << " rows.\n";
  const int dateObsColumn = _file->GetTableColumnIndex("DATE-OBS"),
            dataColumn = _file->GetTableColumnIndex("DATA"),
            freqValColumn = _file->GetTableColumnIndex("CRVAL1"),
            freqRefPixColumn = _file->GetTableColumnIndex("CRPIX1"),
            freqDeltaColumn = _file->GetTableColumnIndex("CDELT1"),
            freqResColumn = _file->GetTableColumnIndex("FREQRES"),
            freqBandwidthColumn = _file->GetTableColumnIndex("BANDWID");
  int timeColumn;
  const bool hasTime = _file->HasTableColumn("TIME", timeColumn);  // optional
  int flagColumn;
  const bool hasFlags =
      _file->HasTableColumn("FLAGGED", flagColumn);  // optional
  int ifColumn;
  const bool hasIF = _file->HasTableColumn("IF", ifColumn);
  if (!hasIF) ifColumn = _file->GetTableColumnIndex("IFNUM");
  std::vector<long> axisDims = _file->GetColumnDimensions(dataColumn);
  int freqCount = 0, polarizationCount = 0, raCount = 0, decCount = 0;
  for (size_t i = 0; i != axisDims.size(); ++i) {
    const std::string name = _file->GetTableDimensionName(i);
    if (name == "FREQ")
      freqCount = axisDims[i];
    else if (name == "STOKES")
      polarizationCount = axisDims[i];
    else if (name == "RA")
      raCount = axisDims[i];
    else if (name == "DEC")
      decCount = axisDims[i];
  }
  const int totalSize = _file->GetTableColumnArraySize(dataColumn);
  if (freqCount == 0) {
    freqCount = totalSize;
    polarizationCount = 1;
    raCount = 1;
    decCount = 1;
  }

  const std::string telescopeName = _file->GetKeywordValue("TELESCOP");
  _antennaInfos[0].name = telescopeName;

  Logger::Debug << "Shape of data cells: " << freqCount << " channels x "
                << polarizationCount << " pols x " << raCount << " RAs x "
                << decCount << " decs"
                << "=" << totalSize << '\n';
  std::vector<long double> cellData(totalSize);
  std::unique_ptr<bool[]> flagData(new bool[totalSize]);
  for (int i = 0; i != totalSize; ++i) flagData[i] = false;
  std::vector<Image2DPtr> images(polarizationCount);
  std::vector<Mask2DPtr> masks(polarizationCount);
  for (int i = 0; i < polarizationCount; ++i) {
    images[i] = Image2D::CreateZeroImagePtr(rowCount, freqCount);
    masks[i] = Mask2D::CreateSetMaskPtr<true>(rowCount, freqCount);
  }
  std::vector<double> observationTimes(rowCount);
  bool hasBand = false;
  const int requestedIFNumber = _bandIndexToNumber[ifIndex];
  size_t timeIndex = 0;
  for (int row = 1; row <= rowCount; ++row) {
    long double time, date, ifNumber;
    _file->ReadTableCell(row, ifColumn, &ifNumber, 1);

    if (ifNumber == requestedIFNumber) {
      if (hasTime) {
        _file->ReadTableCell(row, timeColumn, &time, 1);
        observationTimes[timeIndex] = time;
      }
      _file->ReadTableCell(row, dateObsColumn, &date, 1);
      _file->ReadTableCell(row, dataColumn, &cellData[0], totalSize);
      if (hasFlags)
        _file->ReadTableCell(row, flagColumn, &flagData[0], totalSize);

      if (!hasBand) {
        long double freqVal = 0.0, freqRefPix = 0.0, freqDelta = 0.0,
                    freqRes = 0.0, freqBandwidth = 0.0;
        _file->ReadTableCell(row, freqValColumn, &freqVal, 1);
        _file->ReadTableCell(row, freqRefPixColumn, &freqRefPix, 1);
        _file->ReadTableCell(row, freqDeltaColumn, &freqDelta, 1);
        _file->ReadTableCell(row, freqResColumn, &freqRes, 1);
        _file->ReadTableCell(row, freqBandwidthColumn, &freqBandwidth, 1);
        if (freqBandwidth > 0.0) {
          Logger::Debug << "Frequency info: " << freqVal << " Hz at index "
                        << freqRefPix << ", delta " << freqDelta << "\n";
          Logger::Debug << "Frequency res: " << freqRes << " with bandwidth "
                        << freqBandwidth << " Hz\n";
          BandInfo bandInfo;
          bandInfo.windowIndex = ifNumber;
          for (int i = 0; i < freqCount; ++i) {
            ChannelInfo c;
            c.frequencyIndex = i;
            c.frequencyHz = ((double)i - freqRefPix) * freqDelta + freqVal;
            bandInfo.channels.push_back(c);
          }
          _bandInfos[ifNumber] = bandInfo;
          metaData.SetBand(bandInfo);
          hasBand = true;
        }
      }

      long double* dataPtr = &cellData[0];
      bool* flagPtr = flagData.get();
      for (int p = 0; p < polarizationCount; ++p) {
        for (int f = 0; f < freqCount; ++f) {
          images[p]->SetValue(timeIndex, f, *dataPtr);
          masks[p]->SetValue(timeIndex, f, *flagPtr);
          ++dataPtr;
          ++flagPtr;
        }
      }
      ++timeIndex;
    }
  }
  flagData.reset();
  if (timeIndex == 0) {
    throw std::runtime_error(
        "Couldn't find any rows in the fits image set for the requested IF");
  }
  for (int p = 0; p < polarizationCount; ++p) {
    images[p]->SetTrim(0, 0, timeIndex, images[p]->Height());
    masks[p].reset(
        new Mask2D(masks[p]->Trim(0, 0, timeIndex, images[p]->Height())));
  }
  if (hasTime) {
    observationTimes.resize(timeIndex);
    metaData.SetObservationTimes(observationTimes);
  }
  if (polarizationCount == 1) {
    data = TimeFrequencyData(TimeFrequencyData::AmplitudePart,
                             aocommon::Polarization::StokesI, images[0]);
    data.SetGlobalMask(masks[0]);
  } else if (polarizationCount == 2) {
    data = TimeFrequencyData(TimeFrequencyData::AmplitudePart,
                             aocommon::Polarization::XX, images[0],
                             aocommon::Polarization::YY, images[1]);
    data.SetIndividualPolarizationMasks(masks[0], masks[1]);
  } else {
    std::ostringstream s;
    s << "SDFits file has " << polarizationCount
      << " polarizations: don't know how to convert these";
    throw std::runtime_error(s.str());
  }
}

void FitsImageSet::AddWriteFlagsTask(const ImageSetIndex& index,
                                     std::vector<Mask2DCPtr>& flags) {
  switch (_fitsType) {
    case UVFitsType:
      throw std::runtime_error("Not implemented for UV fits files");
    case SDFitsType:
      saveSingleDishFlags(flags, index.Value() % _bandCount);
      break;
    case DynSpectrumType:
      saveDynSpectrumFlags(flags);
      break;
  }
}

void FitsImageSet::PerformWriteFlagsTask() {
  switch (_fitsType) {
    case UVFitsType:
      throw std::runtime_error(
          "Writing flags not implemented for UV fits files");
    case SDFitsType:
    case DynSpectrumType:
      // Nothing to be done; Add..Task already wrote the flags.
      break;
  }
}

void FitsImageSet::saveSingleDishFlags(const std::vector<Mask2DCPtr>& flags,
                                       size_t ifIndex) {
  _file->Close();
  _file->Open(FitsFile::ReadWriteMode);
  _file->MoveToHDU(2);
  Logger::Debug << "Writing single dish table for band " << ifIndex << " with "
                << _file->GetRowCount() << " rows.\n";
  const int dataColumn = _file->GetTableColumnIndex("DATA"),
            flagColumn = _file->GetTableColumnIndex("FLAGGED");
  int ifColumn = 0;
  const bool hasIF = _file->HasTableColumn("IF", ifColumn);
  if (!hasIF) ifColumn = _file->GetTableColumnIndex("IFNUM");
  const int freqCount = _file->GetColumnDimensionSize(dataColumn, 0),
            polarizationCount = _file->GetColumnDimensionSize(dataColumn, 1);

  const int totalSize = _file->GetTableColumnArraySize(dataColumn);
  const int rowCount = _file->GetRowCount();
  std::vector<double> cellData(totalSize);
  const std::unique_ptr<bool[]> flagData(new bool[totalSize]);
  std::vector<Mask2DCPtr> storedFlags = flags;
  if (flags.size() == 1) {
    while (storedFlags.size() < (unsigned)polarizationCount)
      storedFlags.push_back(flags[0]);
  }
  if (storedFlags.size() != (unsigned)polarizationCount) {
    std::stringstream s;
    s << "saveSingleDishFlags() : mismatch in polarization count: the given "
         "vector contains "
      << flags.size()
      << " polarizations, the number of polarizations in the file is "
      << polarizationCount;
    throw std::runtime_error(s.str());
  }
  for (std::vector<Mask2DCPtr>::const_iterator i = storedFlags.begin();
       i != storedFlags.end(); ++i) {
    if ((*i)->Height() != (unsigned)freqCount)
      throw std::runtime_error(
          "Frequency count in given mask does not match with the file");
  }
  size_t timeIndex = 0;
  const int specifiedIFNumber = _bandIndexToNumber[ifIndex];
  for (int row = 1; row <= rowCount; ++row) {
    long double ifNumber;
    _file->ReadTableCell(row, ifColumn, &ifNumber, 1);

    if (ifNumber == specifiedIFNumber) {
      Logger::Debug << row << "\n";
      _file->ReadTableCell(row, dataColumn, &cellData[0], totalSize);
      double* dataPtr = &cellData[0];
      bool* flagPtr = flagData.get();

      for (int p = 0; p < polarizationCount; ++p) {
        for (int f = 0; f < freqCount; ++f) {
          if (storedFlags[p]->Value(timeIndex, f)) {
            *flagPtr = true;
            *dataPtr = 1e20;
          } else {
            *flagPtr = false;
          }
          ++dataPtr;
          ++flagPtr;
        }
      }

      _file->WriteTableCell(row, dataColumn, &cellData[0], totalSize);
      _file->WriteTableCell(row, flagColumn, flagData.get(), totalSize);
      ++timeIndex;
    }
  }
}

void FitsImageSet::saveDynSpectrumFlags(const std::vector<Mask2DCPtr>& flags) {
  Logger::Debug << "Writing dynspectrum flags.\n";
  _file->Close();
  _file->Open(FitsFile::ReadWriteMode);
  _file->MoveToHDU(1);

  const size_t width = _file->GetCurrentImageSize(1);
  const size_t height = _file->GetCurrentImageSize(2);
  const size_t npol = _file->GetCurrentImageSize(3);
  const size_t n = width * height * npol;
  std::vector<num_t> buffer(n);
  _file->ReadCurrentImageData(0, buffer.data(), buffer.size());

  std::vector<num_t>::iterator bufferIter = buffer.begin();
  for (size_t j = 0; j != npol; ++j) {
    for (size_t y = 0; y != height; ++y) {
      for (size_t x = 0; x != width; ++x) {
        if (flags[j]->Value(x, y))
          *bufferIter = std::numeric_limits<num_t>::quiet_NaN();
        ++bufferIter;
      }
    }
  }

  _file->WriteImage(0, buffer.data(), buffer.size(),
                    std::numeric_limits<num_t>::quiet_NaN());
}

std::string FitsImageSet::Description(const ImageSetIndex& index) const {
  if (IsDynSpectrumType()) {
    return SourceName();
  } else {
    const size_t baselineIndex = index.Value() / _bandCount;
    const size_t bandIndex = index.Value() % _bandCount;
    const int a1 = Baselines()[baselineIndex].first;
    const int a2 = Baselines()[baselineIndex].second;
    const AntennaInfo info1 = GetAntennaInfo(a1);
    const AntennaInfo info2 = GetAntennaInfo(a2);
    std::stringstream s;
    s << "Correlation " << info1.name << " x " << info2.name << ", band "
      << bandIndex;
    return s.str();
  }
}

std::vector<std::string> FitsImageSet::Files() const {
  return std::vector<std::string>{_file->Filename()};
}

std::string FitsImageSet::TelescopeName() {
  if (_fitsType == SDFitsType) {
    for (int hduIndex = 2; hduIndex <= _file->GetHDUCount(); hduIndex++) {
      _file->MoveToHDU(hduIndex);
      const std::string extName = _file->GetKeywordValue("EXTNAME");
      if (extName == "SINGLE DISH") return _file->GetKeywordValue("TELESCOP");
    }
    return "";
  } else {
    return "DynSpectrum";
  }
}
}  // namespace imagesets
