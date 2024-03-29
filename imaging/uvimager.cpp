#include "uvimager.h"

#include "../structures/image2d.h"
#include "../structures/mask2d.h"
#include "../structures/msmetadata.h"
#include "../structures/spatialmatrixmetadata.h"
#include "../structures/timefrequencydata.h"

#include "../util/integerdomain.h"
#include "../util/stopwatch.h"
#include "../util/ffttools.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

UVImager::UVImager(unsigned long xRes, unsigned long yRes, ImageKind imageKind)
    : _xRes(xRes),
      _yRes(yRes),
      _xResFT(xRes),
      _yResFT(yRes),
      _uvReal(),
      _uvImaginary(),
      _uvWeights(),
      _uvFTReal(),
      _uvFTImaginary(),
      _antennas(nullptr),
      _fields(nullptr),
      _imageKind(imageKind),
      _invertFlagging(false),
      _directFT(false),
      _ignoreBoundWarnings(false) {
  _uvScaling = 0.0001L;  // testing
  Empty();
}

UVImager::~UVImager() { Clear(); }

void UVImager::Clear() {
  if (_antennas != nullptr) {
    delete[] _antennas;
    _antennas = nullptr;
  }
  if (_fields != nullptr) {
    delete[] _fields;
    _fields = nullptr;
  }
}

void UVImager::Empty() {
  Clear();
  _uvReal = Image2D::MakeZeroImage(_xRes, _yRes);
  _uvImaginary = Image2D::MakeZeroImage(_xRes, _yRes);
  _uvWeights = Image2D::MakeZeroImage(_xRes, _yRes);
  _uvFTReal = Image2D::MakeZeroImage(_xRes, _yRes);
  _uvFTImaginary = Image2D::MakeZeroImage(_xRes, _yRes);
}

void UVImager::Image(MSMetaData& msMetaData, unsigned band) {
  const unsigned frequencyCount = msMetaData.FrequencyCount(band);
  const IntegerDomain frequencies(0, frequencyCount);
  _msMetaData = &msMetaData;
  _band = _msMetaData->GetBandInfo(band);

  Image(frequencies);
}

void UVImager::Image(class MSMetaData& msMetaData, unsigned band,
                     const IntegerDomain& frequencies) {
  _msMetaData = &msMetaData;
  _band = _msMetaData->GetBandInfo(band);

  Image(frequencies);
}

void UVImager::Image(const IntegerDomain& frequencies) {
  Empty();

  _antennaCount = _msMetaData->AntennaCount();
  _antennas = new AntennaInfo[_antennaCount];
  for (unsigned i = 0; i < _antennaCount; ++i)
    _antennas[i] = _msMetaData->GetAntennaInfo(i);

  _fieldCount = _msMetaData->FieldCount();
  _fields = new FieldInfo[_fieldCount];
  for (unsigned i = 0; i < _fieldCount; ++i)
    _fields[i] = _msMetaData->GetFieldInfo(i);

  const unsigned parts = (frequencies.ValueCount() - 1) / 48 + 1;
  for (unsigned i = 0; i < parts; ++i) {
    std::cout << "Imaging " << i << "/" << parts << ":"
              << frequencies.Split(parts, i).ValueCount() << " frequencies..."
              << std::endl;
    Image(frequencies.Split(parts, i), IntegerDomain(0, _antennaCount),
          IntegerDomain(0, _antennaCount));
  }
}

/**
 * Add several frequency channels to the uv plane for several combinations
 * of antenna pairs.
 */
void UVImager::Image(const IntegerDomain& frequencies,
                     const IntegerDomain& antenna1Domain,
                     const IntegerDomain& antenna2Domain) {
  _scanCount = _msMetaData->TimestepCount();
  std::cout << "Requesting " << frequencies.ValueCount() << " x "
            << antenna1Domain.ValueCount() << " x "
            << antenna2Domain.ValueCount() << " x " << _scanCount << " x "
            << sizeof(SingleFrequencySingleBaselineData) << " = "
            << (frequencies.ValueCount() * antenna1Domain.ValueCount() *
                antenna2Domain.ValueCount() * _scanCount *
                sizeof(SingleFrequencySingleBaselineData) / (1024 * 1024))
            << "MB of memory..." << std::endl;
  SingleFrequencySingleBaselineData**** data =
      new SingleFrequencySingleBaselineData***[frequencies.ValueCount()];
  for (unsigned f = 0; f < frequencies.ValueCount(); ++f) {
    data[f] =
        new SingleFrequencySingleBaselineData**[antenna1Domain.ValueCount()];
    for (unsigned a1 = 0; a1 < antenna1Domain.ValueCount(); ++a1) {
      data[f][a1] =
          new SingleFrequencySingleBaselineData*[antenna2Domain.ValueCount()];
      for (unsigned a2 = 0; a2 < antenna2Domain.ValueCount(); ++a2) {
        data[f][a1][a2] = new SingleFrequencySingleBaselineData[_scanCount];
        for (unsigned scan = 0; scan < _scanCount; ++scan) {
          data[f][a1][a2][scan].flag = true;
          data[f][a1][a2][scan].available = false;
        }
      }
    }
  }

  std::cout << "Reading all data for " << frequencies.ValueCount()
            << " frequencies..." << std::flush;
  Stopwatch stopwatch(true);

  const casacore::MeasurementSet ms(_msMetaData->Path());
  const casacore::ScalarColumn<int> antenna1Col(
      ms,
      casacore::MeasurementSet::columnName(casacore::MeasurementSet::ANTENNA1));
  const casacore::ScalarColumn<int> antenna2Col(
      ms,
      casacore::MeasurementSet::columnName(casacore::MeasurementSet::ANTENNA2));
  const casacore::ScalarColumn<int> fieldIdCol(
      ms,
      casacore::MeasurementSet::columnName(casacore::MeasurementSet::FIELD_ID));
  const casacore::ScalarColumn<int> dataDescIdCol(
      ms, casacore::MeasurementSet::columnName(
              casacore::MeasurementSet::DATA_DESC_ID));
  const casacore::ScalarColumn<double> timeCol(
      ms, casacore::MeasurementSet::columnName(casacore::MeasurementSet::TIME));
  const casacore::ScalarColumn<double> scanNumberCol(
      ms, casacore::MeasurementSet::columnName(
              casacore::MeasurementSet::SCAN_NUMBER));
  const casacore::ArrayColumn<casacore::Complex> correctedDataCol(
      ms, casacore::MeasurementSet::columnName(
              casacore::MeasurementSet::CORRECTED_DATA));
  const casacore::ArrayColumn<bool> flagCol(
      ms, casacore::MeasurementSet::columnName(casacore::MeasurementSet::FLAG));

  const size_t rows = ms.nrow();
  for (unsigned row = 0; row != rows; ++row) {
    const unsigned a1 = antenna1Col(row);
    const unsigned a2 = antenna2Col(row);
    if (antenna1Domain.IsIn(a1) && antenna2Domain.IsIn(a2)) {
      const unsigned scan = scanNumberCol(row);
      const unsigned index1 = antenna1Domain.Index(a1);
      const unsigned index2 = antenna1Domain.Index(a2);
      const int field = fieldIdCol(row);
      const double time = timeCol(row);
      casacore::Array<casacore::Complex> dataArr = correctedDataCol(row);
      casacore::Array<bool> flagArr = flagCol(row);
      casacore::Array<casacore::Complex>::const_iterator cdI = dataArr.begin();
      casacore::Array<bool>::const_iterator fI = flagArr.begin();
      for (int f = 0; f < frequencies.GetValue(0); ++f) {
        ++fI;
        ++fI;
        ++fI;
        ++fI;
        ++cdI;
        ++cdI;
        ++cdI;
        ++cdI;
      }
      for (unsigned f = 0; f < frequencies.ValueCount(); ++f) {
        SingleFrequencySingleBaselineData& curData =
            data[f][index1][index2][scan];
        const casacore::Complex xxData = *cdI;
        ++cdI;
        ++cdI;
        ++cdI;
        const casacore::Complex yyData = *cdI;
        ++cdI;
        curData.data = xxData + yyData;
        bool flagging = *fI;
        ++fI;
        ++fI;
        ++fI;
        flagging = flagging || *fI;
        ++fI;
        curData.flag = flagging;
        curData.field = field;
        curData.time = time;
        curData.available = true;
      }
    }
  }
  stopwatch.Pause();
  std::cout << "DONE in " << stopwatch.ToString() << " ("
            << (stopwatch.Seconds() /
                (antenna1Domain.ValueCount() * antenna1Domain.ValueCount()))
            << "s/antenna)" << std::endl;

  std::cout << "Imaging..." << std::flush;
  stopwatch.Reset();
  stopwatch.Start();
  for (unsigned f = 0; f < frequencies.ValueCount(); ++f) {
    for (unsigned a1 = 0; a1 < antenna1Domain.ValueCount(); ++a1) {
      for (unsigned a2 = 0; a2 < antenna2Domain.ValueCount(); ++a2) {
        Image(frequencies.GetValue(f), _antennas[antenna1Domain.GetValue(a1)],
              _antennas[antenna2Domain.GetValue(a2)], data[f][a1][a2]);
      }
    }
  }
  stopwatch.Pause();
  std::cout << "DONE in " << stopwatch.ToString() << " ("
            << (stopwatch.Seconds() /
                (antenna1Domain.ValueCount() * antenna1Domain.ValueCount()))
            << "s/antenna)" << std::endl;

  // free data
  for (unsigned f = 0; f < frequencies.ValueCount(); ++f) {
    for (unsigned a1 = 0; a1 < antenna1Domain.ValueCount(); ++a1) {
      for (unsigned a2 = 0; a2 < antenna2Domain.ValueCount(); ++a2) {
        delete[] data[f][a1][a2];
      }
      delete[] data[f][a1];
    }
    delete[] data[f];
  }
  delete[] data;
}

void UVImager::Image(unsigned frequencyIndex, AntennaInfo& antenna1,
                     AntennaInfo& antenna2,
                     SingleFrequencySingleBaselineData* data) {
  const num_t frequency = _band.channels[frequencyIndex].frequencyHz;
  const num_t speedOfLight = 299792458.0L;
  AntennaCache cache;
  cache.wavelength = speedOfLight / frequency;

  // dx, dy, dz is the baseline
  cache.dx = antenna1.position.x - antenna2.position.x;
  cache.dy = antenna1.position.y - antenna2.position.y;
  cache.dz = antenna1.position.z - antenna2.position.z;

  for (unsigned i = 0; i < _scanCount; ++i) {
    if (data[i].available) {
      switch (_imageKind) {
        case Homogeneous:
          if (!data[i].flag) {
            num_t u, v;
            GetUVPosition(u, v, data[i], cache);
            SetUVValue(u, v, data[i].data.real(), data[i].data.imag(), 1.0);
            SetUVValue(-u, -v, data[i].data.real(), -data[i].data.imag(), 1.0);
            // calcTimer.Pause();
          }
          break;
        case Flagging:
          if ((data[i].flag && !_invertFlagging) ||
              (!data[i].flag && _invertFlagging)) {
            num_t u, v;
            GetUVPosition(u, v, data[i], cache);
            SetUVValue(u, v, 1, 0, 1.0);
            SetUVValue(-u, -v, 1, 0, 1.0);
          }
          break;
      }
    }
  }
}

void UVImager::Image(const class TimeFrequencyData& data,
                     class SpatialMatrixMetaData* metaData) {
  if (!_uvReal.Empty()) Empty();
  Image2DCPtr real = data.GetRealPart(), imaginary = data.GetImaginaryPart();
  const Mask2DCPtr flags = data.GetSingleMask();

  for (unsigned a2 = 0; a2 < data.ImageHeight(); ++a2) {
    for (unsigned a1 = a2 + 1; a1 < data.ImageWidth(); ++a1) {
      num_t vr = real->Value(a1, a2), vi = imaginary->Value(a1, a2);
      if (std::isfinite(vr) && std::isfinite(vi)) {
        const UVW uvw = metaData->UVW(a1, a2);
        SetUVValue(uvw.u, uvw.v, vr, vi, 1.0);
        SetUVValue(-uvw.u, -uvw.v, vr, -vi, 1.0);
      }
    }
  }
}

void UVImager::Image(const TimeFrequencyData& data,
                     TimeFrequencyMetaDataCPtr metaData,
                     unsigned frequencyIndex) {
  if (!_uvReal.Empty()) Empty();

  Image2DCPtr real = data.GetRealPart(), imaginary = data.GetImaginaryPart();
  const Mask2DCPtr flags = data.GetSingleMask();

  for (unsigned i = 0; i < data.ImageWidth(); ++i) {
    switch (_imageKind) {
      case Homogeneous:
        if (flags->Value(i, frequencyIndex) == 0.0L) {
          num_t vr = real->Value(i, frequencyIndex),
                vi = imaginary->Value(i, frequencyIndex);
          if (std::isfinite(vr) && std::isfinite(vi)) {
            num_t u, v;
            GetUVPosition(u, v, i, frequencyIndex, metaData);
            SetUVValue(u, v, vr, vi, 1.0);
            SetUVValue(-u, -v, vr, -vi, 1.0);
          }
        }
        break;
      case Flagging:
        if ((flags->Value(i, frequencyIndex) != 0.0L && !_invertFlagging) ||
            (flags->Value(i, frequencyIndex) == 0.0L && _invertFlagging)) {
          num_t u, v;
          GetUVPosition(u, v, i, frequencyIndex, metaData);
          SetUVValue(u, v, 1, 0, 1.0);
          SetUVValue(-u, -v, 1, 0, 1.0);
        }
        break;
    }
  }
}

void UVImager::ApplyWeightsToUV() {
  const double normFactor =
      _uvWeights.Sum() / ((num_t)_uvReal.Height() * _uvReal.Width());
  for (size_t y = 0; y < _uvReal.Height(); ++y) {
    for (size_t x = 0; x < _uvReal.Width(); ++x) {
      const num_t weight = _uvWeights.Value(x, y);
      if (weight != 0.0) {
        _uvReal.SetValue(x, y, _uvReal.Value(x, y) * normFactor / weight);
        _uvImaginary.SetValue(x, y,
                              _uvImaginary.Value(x, y) * normFactor / weight);
        _uvWeights.SetValue(x, y, 1.0);
      }
    }
  }
  _uvFTReal = Image2D();
  _uvFTImaginary = Image2D();
}

void UVImager::SetUVValue(num_t u, num_t v, num_t r, num_t i, num_t weight) {
  // Nearest neighbour interpolation
  const long uPos = (long)floorn(u * _uvScaling * _xRes + 0.5) + (_xRes / 2);
  const long vPos = (long)floorn(v * _uvScaling * _yRes + 0.5) + (_yRes / 2);
  if (uPos >= 0 && uPos < (long)_xRes && vPos >= 0 && vPos < (long)_yRes) {
    _uvReal.AddValue(uPos, vPos, r);
    _uvImaginary.AddValue(uPos, vPos, i);
    _uvWeights.AddValue(uPos, vPos, weight);
  } else {
    if (!_ignoreBoundWarnings) {
      std::cout << "Warning! Baseline outside uv window (" << uPos << ","
                << vPos << ")."
                << "(subsequent out of bounds warnings will not be noted)"
                << std::endl;
      _ignoreBoundWarnings = true;
    }
  }
  // Linear interpolation
  /*long uPos = (long) floor(u*_uvScaling*_xRes+0.5L) + _xRes/2;
  long vPos = (long) floor(v*_uvScaling*_yRes+0.5L) + _yRes/2;
  if(uPos>=0 && uPos<(long) _xRes && vPos>=0 && vPos<(long) _yRes) {
          long double dx = (long double) uPos - (_xRes/2) -
  (u*_uvScaling*_xRes+0.5L); long double dy = (long double) vPos - (_yRes/2) -
  (v*_uvScaling*_yRes+0.5L); long double distance = sqrtn(dx*dx + dy*dy);
          if(distance > 1.0) distance = 1.0;
          weight *= distance;
          _uvReal.AddValue(uPos, vPos, r*weight);
          _uvImaginary.AddValue(uPos, vPos, i*weight);
          _uvWeights.AddValue(uPos, vPos, weight);
  } else {
          std::cout << "Warning! Baseline outside uv window (" << uPos << "," <<
  vPos << ")." << std::endl;
  }*/
}

void UVImager::SetUVFTValue(num_t u, num_t v, num_t r, num_t i, num_t weight) {
  for (size_t iy = 0; iy < _yResFT; ++iy) {
    for (size_t ix = 0; ix < _xResFT; ++ix) {
      const num_t x =
          ((num_t)ix - (_xResFT / 2)) / _uvScaling * _uvFTReal.Width();
      const num_t y =
          ((num_t)iy - (_yResFT / 2)) / _uvScaling * _uvFTReal.Height();
      // Calculate F(x,y) += f(u, v) e ^ {i 2 pi (x u + y v) }
      const num_t fftRotation = (u * x + v * y) * -2.0L * M_PIn;
      num_t fftCos = cosn(fftRotation), fftSin = sinn(fftRotation);
      _uvFTReal.AddValue(ix, iy, (fftCos * r - fftSin * i) * weight);
      _uvFTImaginary.AddValue(ix, iy, (fftSin * r + fftCos * i) * weight);
    }
  }
}

void UVImager::PerformFFT() {
  if (!_uvFTReal.Empty()) {
    _uvFTReal = Image2D::MakeZeroImage(_xRes, _yRes);
    _uvFTImaginary = Image2D::MakeZeroImage(_xRes, _yRes);
  }
  FFTTools::CreateFFTImage(_uvReal, _uvImaginary, _uvFTReal, _uvFTImaginary);
}

void UVImager::GetUVPosition(num_t& u, num_t& v, size_t timeIndex,
                             size_t frequencyIndex,
                             TimeFrequencyMetaDataCPtr metaData) {
  const num_t frequency = metaData->Band().channels[frequencyIndex].frequencyHz;
  u = metaData->UVW()[timeIndex].u * frequency / SpeedOfLight();
  v = metaData->UVW()[timeIndex].v * frequency / SpeedOfLight();
}

void UVImager::GetUVPosition(num_t& u, num_t& v,
                             const SingleFrequencySingleBaselineData& data,
                             const AntennaCache& cache) {
  const unsigned field = data.field;
  const num_t pointingLattitude = _fields[field].delayDirectionRA;
  const num_t pointingLongitude = _fields[field].delayDirectionDec;

  // calcTimer.Start();
  const num_t earthLattitudeAngle =
      Date::JDToHourOfDay(Date::AipsMJDToJD(data.time)) * M_PIn / 12.0L;

  // long double pointingLongitude = _fields[field].delayDirectionDec; //not
  // used

  // Rotate baseline plane towards source, first rotate around z axis, then
  // around x axis
  const num_t raRotation =
      earthLattitudeAngle - pointingLattitude + M_PIn * 0.5L;
  num_t tmpCos = cosn(raRotation);
  num_t tmpSin = sinn(raRotation);

  const num_t dxProjected = tmpCos * cache.dx - tmpSin * cache.dy;
  const num_t tmpdy = tmpSin * cache.dx + tmpCos * cache.dy;

  tmpCos = cosn(-pointingLongitude);
  tmpSin = sinn(-pointingLongitude);
  const num_t dyProjected = tmpCos * tmpdy - tmpSin * cache.dz;
  // long double dzProjected = tmpSin*tmpdy + tmpCos*dzAnt; // we don't need it

  // Now, the newly projected positive z axis of the baseline points to the
  // field

  num_t baselineLength =
      sqrtn(dxProjected * dxProjected + dyProjected * dyProjected);

  num_t baselineAngle;
  if (baselineLength == 0.0L) {
    baselineAngle = 0.0L;
  } else {
    baselineLength /= cache.wavelength;
    if (dxProjected > 0.0L)
      baselineAngle = atann(dyProjected / dxProjected);
    else
      baselineAngle = M_PIn - atann(dyProjected / -dxProjected);
  }

  u = cosn(baselineAngle) * baselineLength;
  v = -sinn(baselineAngle) * baselineLength;
}

num_t UVImager::GetFringeStopFrequency(size_t timeIndex,
                                       const Baseline& /*baseline*/,
                                       num_t /*delayDirectionRA*/,
                                       num_t delayDirectionDec,
                                       num_t /*frequency*/,
                                       TimeFrequencyMetaDataCPtr metaData) {
  // earthspeed = rad / sec
  const num_t earthSpeed = 2.0L * M_PIn / (24.0L * 60.0L * 60.0L);
  // num_t earthLattitudeAngle =
  //	Date::JDToHourOfDay(Date::AipsMJDToJD(metaData->ObservationTimes()[timeIndex]))*M_PIn/12.0L;
  // num_t raSin = sinn(-delayDirectionRA - earthLattitudeAngle);
  // num_t raCos = cosn(-delayDirectionRA - earthLattitudeAngle);
  // num_t dx = baseline.antenna2.x - baseline.antenna1.x;
  // num_t dy = baseline.antenna2.y - baseline.antenna1.y;
  // num_t wavelength = 299792458.0L / frequency;
  return -earthSpeed * metaData->UVW()[timeIndex].u * cosn(delayDirectionDec);
}

num_t UVImager::GetFringeCount(size_t timeIndexStart, size_t timeIndexEnd,
                               unsigned channelIndex,
                               const TimeFrequencyMetaDataCPtr metaData) {
  // For now, I've made this return the negative fringe count, because it does
  // not match with the fringe stop frequency returned above otherwise; probably
  // because of a mismatch in the signs of u,v,w somewhere...
  return -(metaData->UVW()[timeIndexEnd].w -
           metaData->UVW()[timeIndexStart].w) *
         metaData->Band().channels[channelIndex].frequencyHz / 299792458.0L;
}

void UVImager::InverseImage(MSMetaData& prototype, unsigned band,
                            const Image2D& /*uvReal*/,
                            const Image2D& /*uvImaginary*/,
                            unsigned antenna1Index, unsigned antenna2Index) {
  _timeFreq = Image2D::MakeZeroImage(prototype.TimestepCount(),
                                     prototype.FrequencyCount(band));
  AntennaInfo antenna1, antenna2;
  antenna1 = prototype.GetAntennaInfo(antenna1Index);
  antenna2 = prototype.GetAntennaInfo(antenna2Index);
}
