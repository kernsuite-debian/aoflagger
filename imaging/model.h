#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <cmath>
#include <utility>

#include "../structures/image2d.h"

#include "../structures/types.h"

#include "uvimager.h"

template <typename T>
struct OutputReceiver {
  void SetY(size_t) {}
};
template <>
struct OutputReceiver<UVImager> {
  UVImager* _imager;
  void SetUVValue(size_t, double u, double v, double r, double i, double w) {
    _imager->SetUVValue(u, v, r, i, w);
    _imager->SetUVValue(-u, -v, r, -i, w);
  }
  void SetY(size_t) {}
};
template <>
struct OutputReceiver<TimeFrequencyData> {
  Image2DPtr _real, _imaginary;
  size_t _y;
  void SetUVValue(size_t x, double, double, double r, double i, double) {
    _real->SetValue(x, _y, r);
    _imaginary->SetValue(x, _y, i);
  }
  void SetY(size_t y) { _y = y; }
};

class Model {
  struct Source {
    virtual ~Source() {}
    virtual numl_t Dec(num_t t) const = 0;
    virtual numl_t Ra(num_t t) const = 0;
    virtual numl_t FluxIntensity(num_t t) const = 0;
    virtual numl_t SqrtFluxIntensity(num_t t) const {
      return sqrtnl(FluxIntensity(t));
    }
  };
  struct StablePointSource final : public Source {
    long double dec, ra, fluxIntensity, sqrtFluxIntensity;
    numl_t Dec(num_t) const override { return dec; }
    numl_t Ra(num_t) const override { return ra; }
    numl_t FluxIntensity(num_t) const override { return fluxIntensity; }
    numl_t SqrtFluxIntensity(num_t) const override { return sqrtFluxIntensity; }
  };
  struct VariablePointSource final : public Source {
    long double dec, ra, fluxIntensity;
    double peakTime, oneOverSigmaSq;
    numl_t Dec(num_t) const override { return dec; }
    numl_t Ra(num_t) const override { return ra; }
    numl_t FluxIntensity(num_t t) const override {
      numl_t mu = fmodnl(fabsnl(t - peakTime), 1.0);
      if (mu > 0.5) mu = 1.0 - mu;
      return fluxIntensity * (1.0 + expnl(mu * mu * oneOverSigmaSq)) *
             (1.0 + fmod(t * 1007.0, 13.0) / 26.0);
    }
  };

 public:
  Model();
  void AddSource(long double dec, long double ra, long double fluxIntensity) {
    StablePointSource* source = new StablePointSource();
    source->dec = dec;
    source->ra = ra;
    source->fluxIntensity = fluxIntensity;
    source->sqrtFluxIntensity = sqrt(fluxIntensity);
    _sources.push_back(source);
  }
  void AddVariableSource(long double dec, long double ra,
                         long double fluxIntensity) {
    VariablePointSource* source = new VariablePointSource();
    source->dec = dec;
    source->ra = ra;
    source->fluxIntensity = fluxIntensity;
    source->peakTime = 0.2;
    source->oneOverSigmaSq = 1.0 / (0.3 * 0.3);
    _sources.push_back(source);
  }
  void SimulateAntenna(double time, num_t delayDirectionDEC,
                       num_t delayDirectionRA, num_t dx, num_t dy,
                       num_t frequency, num_t earthLattitude, num_t& r,
                       num_t& i);
  void SimulateUncoherentAntenna(double time, num_t delayDirectionDEC,
                                 num_t delayDirectionRA, num_t dx, num_t dy,
                                 num_t frequency, num_t earthLattitude,
                                 num_t& r, num_t& i, size_t index);

  template <typename T>
  void SimulateCorrelation(struct OutputReceiver<T>& receiver,
                           num_t delayDirectionDEC, num_t delayDirectionRA,
                           num_t dx, num_t dy, num_t dz, num_t frequency,
                           num_t channelWidth, size_t nTimes,
                           double integrationTime);

  void SimulateObservation(class UVImager& imager,
                           class Observatorium& observatorium,
                           num_t delayDirectionDEC, num_t delayDirectionRA) {
    srand(1);
    OutputReceiver<UVImager> imagerOutputter;
    imagerOutputter._imager = &imager;
    SimulateObservation(imagerOutputter, observatorium, delayDirectionDEC,
                        delayDirectionRA);
  }

  std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr> SimulateObservation(
      size_t nTimes, class Observatorium& observatorium,
      num_t delayDirectionDEC, num_t delayDirectionRA, size_t a1, size_t a2);

  template <typename T>
  void SimulateObservation(struct OutputReceiver<T>& receiver,
                           class Observatorium& observatorium,
                           num_t delayDirectionDEC, num_t delayDirectionRA);

  static void GetUVPosition(num_t& u, num_t& v, num_t earthLattitudeAngle,
                            num_t delayDirectionDEC, num_t delayDirectionRA,
                            num_t dx, num_t dy, num_t dz, num_t waveLength);
  static num_t GetWPosition(num_t delayDirectionDec, num_t delayDirectionRA,
                            num_t frequency, num_t earthLattitudeAngle,
                            num_t dx, num_t dy) {
    return UVImager::GetWPosition(delayDirectionDec, delayDirectionRA,
                                  frequency, earthLattitudeAngle, dx, dy);
  }

  void loadUrsaMajor(double ra, double dec, double factor);
  void loadUrsaMajorDistortingSource(double ra, double dec, double factor,
                                     bool slightlyMiss = false);
  void loadUrsaMajorDistortingVariableSource(double ra, double dec,
                                             double factor, bool weak = false,
                                             bool slightlyMiss = false);

  double NoiseSigma() const { return _noiseSigma; }
  void SetNoiseSigma(double noiseSigma) { _noiseSigma = noiseSigma; }

 private:
  std::vector<Source*> _sources;
  double _noiseSigma, _sourceSigma;
  double _integrationTime;
};

#endif
