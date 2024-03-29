#ifndef DEFAULTMODELS_H
#define DEFAULTMODELS_H

#include <utility>

#include "model.h"
#include "observatorium.h"

enum class SourceSet {
  NoDistortion,
  ConstantDistortion,
  StrongVariableDistortion,
  FaintVariableDistortion
};

class DefaultModels {
 public:
  enum SetLocation { EmptySet, NCPSet };
  static double DistortionRA() { return 4.940; }

  static double DistortionDec() { return 0.571; }

  static std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr> LoadSet(
      enum SetLocation setLocation, enum SourceSet distortion,
      double noiseSigma, size_t channelCount = 64,
      double bandwidth = 2500000.0 * 16.0, unsigned a1 = 0, unsigned a2 = 5) {
    double ra, dec, factor;
    getSetData(setLocation, ra, dec, factor);
    Model model;
    model.SetNoiseSigma(noiseSigma);
    if (setLocation != EmptySet) model.loadUrsaMajor(ra, dec, factor);
    switch (distortion) {
      case SourceSet::NoDistortion:
        break;
      case SourceSet::ConstantDistortion:
        model.loadUrsaMajorDistortingSource(ra, dec, factor, true);
        break;
      case SourceSet::StrongVariableDistortion:
        model.loadUrsaMajorDistortingVariableSource(ra, dec, factor, false,
                                                    false);
        break;
      case SourceSet::FaintVariableDistortion:
        model.loadUrsaMajorDistortingVariableSource(ra, dec, factor, true,
                                                    false);
        break;
    }
    WSRTObservatorium wsrtObservatorium(channelCount, bandwidth);
    return model.SimulateObservation(12 * 60 * 60 / 15, wsrtObservatorium, dec,
                                     ra, a1, a2);
  }

 private:
  static void getSetData(enum SetLocation setLocation, double& ra, double& dec,
                         double& factor) {
    if (setLocation == NCPSet) {
      dec = 0.5 * M_PI + 0.12800;
      ra = -0.03000;
      factor = 1.0;
    } else {
      dec = 1.083;
      ra = 4.865;
      factor = 4.0;
    }
  }
};

#endif
