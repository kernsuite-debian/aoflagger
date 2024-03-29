#include "model.h"

#include "../structures/image2d.h"
#include "../structures/timefrequencydata.h"

#include "observatorium.h"
#include "../util/rng.h"
#include "../util/logger.h"

#include <iostream>

Model::Model() : _noiseSigma(1.0), _sourceSigma(0.0), _integrationTime(15.0) {}

template <typename T>
void Model::SimulateObservation(struct OutputReceiver<T>& receiver,
                                Observatorium& observatorium,
                                num_t delayDirectionDEC,
                                num_t delayDirectionRA) {
  const size_t channelCount = observatorium.BandInfo().channels.size();
  const double frequency = observatorium.BandInfo().channels[0].frequencyHz;

  for (size_t f = 0; f < channelCount; ++f) {
    const double channelFrequency =
        frequency + observatorium.ChannelWidthHz() * f;
    receiver.SetY(f);
    for (size_t i = 0; i < observatorium.AntennaCount(); ++i) {
      for (size_t j = i + 1; j < observatorium.AntennaCount(); ++j) {
        const AntennaInfo &antenna1 = observatorium.GetAntenna(i),
                          &antenna2 = observatorium.GetAntenna(j);
        double dx = antenna1.position.x - antenna2.position.x,
               dy = antenna1.position.y - antenna2.position.y,
               dz = antenna1.position.z - antenna2.position.z;

        SimulateCorrelation(receiver, delayDirectionDEC, delayDirectionRA, dx,
                            dy, dz, channelFrequency,
                            observatorium.ChannelWidthHz(), 12 * 60 * 60,
                            _integrationTime);
      }
    }
  }
}

template void Model::SimulateObservation(
    struct OutputReceiver<UVImager>& receiver, Observatorium& observatorium,
    num_t delayDirectionDEC, num_t delayDirectionRA);
template void Model::SimulateObservation(
    struct OutputReceiver<TimeFrequencyData>& receiver,
    Observatorium& observatorium, num_t delayDirectionDEC,
    num_t delayDirectionRA);

std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr>
Model::SimulateObservation(size_t nTimes, class Observatorium& observatorium,
                           num_t delayDirectionDEC, num_t delayDirectionRA,
                           size_t a1, size_t a2) {
  const size_t channelCount = observatorium.BandInfo().channels.size();
  const double frequency = observatorium.BandInfo().channels[0].frequencyHz;

  OutputReceiver<TimeFrequencyData> tfOutputter;
  tfOutputter._real = Image2D::CreateZeroImagePtr(nTimes, channelCount);
  tfOutputter._imaginary = Image2D::CreateZeroImagePtr(nTimes, channelCount);

  const TimeFrequencyMetaDataPtr metaData(new TimeFrequencyMetaData());
  metaData->SetAntenna1(observatorium.GetAntenna(a1));
  metaData->SetAntenna2(observatorium.GetAntenna(a2));
  metaData->SetBand(observatorium.BandInfo());
  double dx = metaData->Antenna1().position.x - metaData->Antenna2().position.x,
         dy = metaData->Antenna1().position.y - metaData->Antenna2().position.y,
         dz = metaData->Antenna1().position.z - metaData->Antenna2().position.z;

  for (size_t f = 0; f < channelCount; ++f) {
    const double channelFrequency =
        frequency + observatorium.ChannelWidthHz() * f;
    tfOutputter.SetY(f);
    SimulateCorrelation(tfOutputter, delayDirectionDEC, delayDirectionRA, dx,
                        dy, dz, channelFrequency,
                        observatorium.ChannelWidthHz(), nTimes,
                        _integrationTime);
  }

  std::vector<double> times;
  std::vector<UVW> uvws;
  const num_t wavelength = 1.0L / frequency;
  for (size_t i = 0; i != nTimes; ++i) {
    const double t = _integrationTime * i;
    times.push_back(t);
    const num_t earthLattitudeApprox = t * M_PIn / (12.0 * 60.0 * 60.0);
    UVW uvw;
    GetUVPosition(uvw.u, uvw.v, earthLattitudeApprox, delayDirectionDEC,
                  delayDirectionRA, dx, dy, dz, wavelength);
    uvw.u = uvw.u * (299792458.0L / frequency);
    uvw.v = uvw.v * (299792458.0L / frequency);
    uvw.w = GetWPosition(delayDirectionDEC, delayDirectionRA, frequency,
                         earthLattitudeApprox, dx, dy) *
            (299792458.0L / frequency);
    uvws.push_back(uvw);
  }
  metaData->SetUVW(uvws);
  metaData->SetObservationTimes(times);
  FieldInfo field;
  field.fieldId = 0;
  field.delayDirectionDec = delayDirectionDEC;
  // field.delayDirectionDecNegCos = -cos(delayDirectionDEC);
  // field.delayDirectionDecNegSin = -sin(delayDirectionDEC);
  field.delayDirectionRA = delayDirectionRA;
  metaData->SetField(field);

  const TimeFrequencyData tfData(aocommon::Polarization::StokesI,
                                 tfOutputter._real, tfOutputter._imaginary);
  return std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr>(tfData,
                                                                metaData);
}

template <typename T>
void Model::SimulateCorrelation(struct OutputReceiver<T>& receiver,
                                num_t delayDirectionDEC, num_t delayDirectionRA,
                                num_t dx, num_t dy, num_t dz, num_t frequency,
                                num_t channelWidth, size_t nTimes,
                                double integrationTime) {
  const double sampleGain =
      integrationTime / (12.0 * 60.0 * 60.0) * channelWidth;
  const num_t wavelength = 1.0L / frequency;
  size_t index = 0;
  for (size_t ti = 0; ti != nTimes; ++ti) {
    const double t = ti * integrationTime;
    const double timeInDays = t / (12.0 * 60.0 * 60.0);
    const num_t earthLattitudeApprox = timeInDays * M_PIn;
    num_t u, v, r1, i1, r2, i2;
    GetUVPosition(u, v, earthLattitudeApprox, delayDirectionDEC,
                  delayDirectionRA, dx, dy, dz, wavelength);
    SimulateAntenna(timeInDays, delayDirectionDEC, delayDirectionRA, 0, 0,
                    frequency, earthLattitudeApprox, r1, i1);
    SimulateAntenna(timeInDays, delayDirectionDEC, delayDirectionRA, dx, dy,
                    frequency, earthLattitudeApprox, r2, i2);
    num_t r = r1 * r2 - (i1 * -i2), i = r1 * -i2 + r2 * i1;
    receiver.SetUVValue(index, u, v, r * sampleGain, i * sampleGain, 1.0);
    ++index;
  }
}

template void Model::SimulateCorrelation(
    struct OutputReceiver<UVImager>& receiver, num_t delayDirectionDEC,
    num_t delayDirectionRA, num_t dx, num_t dy, num_t dz, num_t frequency,
    num_t channelWidth, size_t nTimes, double integrationTime);

void Model::SimulateAntenna(double time, num_t delayDirectionDEC,
                            num_t delayDirectionRA, num_t dx, num_t dy,
                            num_t frequency, num_t earthLattitude, num_t& r,
                            num_t& i) {
  r = 0.0;
  i = 0.0;
  const num_t delayW = GetWPosition(delayDirectionDEC, delayDirectionRA,
                                    frequency, earthLattitude, dx, dy);
  for (std::vector<Source*>::const_iterator iter = _sources.begin();
       iter != _sources.end(); ++iter) {
    const Source& source = **iter;
    const num_t w = GetWPosition(source.Dec(time), source.Ra(time), frequency,
                                 earthLattitude, dx, dy);
    const num_t fieldStrength =
        source.SqrtFluxIntensity(time) + RNG::Gaussian() * _sourceSigma;
    num_t noiser, noisei;
    RNG::ComplexGaussianAmplitude(noiser, noisei);
    r +=
        fieldStrength * cosn((w - delayW) * M_PIn * 2.0) + noiser * _noiseSigma;
    i +=
        fieldStrength * sinn((w - delayW) * M_PIn * 2.0) + noisei * _noiseSigma;
  }
}

void Model::SimulateUncoherentAntenna(double time, num_t delayDirectionDEC,
                                      num_t delayDirectionRA, num_t dx,
                                      num_t dy, num_t frequency,
                                      num_t earthLattitude, num_t& r, num_t& i,
                                      size_t index) {
  const num_t delayW = GetWPosition(delayDirectionDEC, delayDirectionRA,
                                    frequency, earthLattitude, dx, dy);

  // if(index%(_sources.size()+1) == _sources.size())
  //{
  num_t noiser, noisei;
  RNG::ComplexGaussianAmplitude(noiser, noisei);
  noiser *= _noiseSigma;
  noisei *= _noiseSigma;
  //}
  // else {
  const Source& source = *_sources[index % _sources.size()];
  const num_t w = GetWPosition(source.Dec(time), source.Ra(time), frequency,
                               earthLattitude, dx, dy);
  const num_t fieldStrength =
      source.SqrtFluxIntensity(time) + RNG::Gaussian() * _sourceSigma;
  r = fieldStrength * cosn((w - delayW) * M_PIn * 2.0) + noiser;
  i = fieldStrength * sinn((w - delayW) * M_PIn * 2.0) + noisei;
  //}
}

void Model::GetUVPosition(num_t& u, num_t& v, num_t earthLattitudeAngle,
                          num_t delayDirectionDEC, num_t delayDirectionRA,
                          num_t dx, num_t dy, num_t dz, num_t wavelength) {
  // Rotate baseline plane towards phase center, first rotate around z axis,
  // then around x axis
  const long double raRotation =
      -earthLattitudeAngle + delayDirectionRA + M_PIn * 0.5L;
  long double tmpCos = cosn(raRotation);
  long double tmpSin = sinn(raRotation);

  const long double dxProjected = tmpCos * dx - tmpSin * dy;
  const long double tmpdy = tmpSin * dx + tmpCos * dy;

  tmpCos = cosn(-delayDirectionDEC);
  tmpSin = sinn(-delayDirectionDEC);
  const long double dyProjected = tmpCos * tmpdy - tmpSin * dz;

  // Now, the newly projected positive z axis of the baseline points to the
  // phase center
  long double baselineLength =
      sqrtn(dxProjected * dxProjected + dyProjected * dyProjected);

  long double baselineAngle;
  if (baselineLength == 0.0) {
    baselineAngle = 0.0;
  } else {
    baselineLength /= 299792458.0L * wavelength;
    if (dxProjected > 0.0L)
      baselineAngle = atann(dyProjected / dxProjected);
    else
      baselineAngle = M_PIn - atann(dyProjected / -dxProjected);
  }

  u = cosn(baselineAngle) * baselineLength;
  v = -sinn(baselineAngle) * baselineLength;
}

void Model::loadUrsaMajor(double ra, double dec, double factor) {
  double s = 0.00005 * factor,  // scale
      rs = 6.0 + 2.0 * factor;  // stretch in dec
  const double fluxoffset = 0.0;

  AddSource(dec + s * rs * 40, ra + s * 72, 8.0 / 8.0 + fluxoffset);   // Dubhe
  AddSource(dec + s * rs * -16, ra + s * 81, 4.0 / 8.0 + fluxoffset);  // Beta
  AddSource(dec + s * rs * -45, ra + s * 2, 3.0 / 8.0 + fluxoffset);   // Gamma
  AddSource(dec + s * rs * -6, ra + s * -27, 2.0 / 8.0 + fluxoffset);  // Delta
  AddSource(dec + s * rs * -4, ra + s * -85, 6.0 / 8.0 + fluxoffset);  // Alioth
  AddSource(dec + s * rs * 2, ra + s * -131, 5.0 / 8.0 + fluxoffset);  // Zeta
  AddSource(dec + s * rs * -36, ra + s * -192,
            7.0 / 8.0 + fluxoffset);  // Alkaid

  // AddSource(cd, cr - M_PI, 4.0);
}

void Model::loadUrsaMajorDistortingSource(double ra, double dec, double factor,
                                          bool slightlyMiss) {
  if (slightlyMiss) {
    dec += 0.005;
    ra += 0.002;
  }
  AddSource(dec - 0.12800 * factor, ra + 0.015 + 0.015 * factor, 4.0);
}

void Model::loadUrsaMajorDistortingVariableSource(double ra, double dec,
                                                  double factor, bool weak,
                                                  bool slightlyMiss) {
  double flux = 4.0;
  dec = dec - 0.12800 * factor;
  ra = ra + 0.015 + 0.015 * factor;
  if (slightlyMiss) {
    dec += 0.005;
    ra += 0.002;
  }
  if (weak) {
    flux /= 100.0;
  }
  AddVariableSource(dec, ra, flux);
}
