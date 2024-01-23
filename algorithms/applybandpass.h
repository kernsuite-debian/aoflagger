#ifndef APPLY_BANDPASS_H
#define APPLY_BANDPASS_H

#include "../structures/timefrequencydata.h"

#include "bandpassfile.h"

#include <string>
#include <utility>

namespace algorithms {

class ApplyBandpass {
 public:
  static void Apply(TimeFrequencyData& data, BandpassFile& file,
                    const std::string& antenna1, const std::string& antenna2) {
    for (size_t i = 0; i != data.PolarizationCount(); ++i) {
      TimeFrequencyData polData = data.MakeFromPolarizationIndex(i);
      char pol1, pol2;
      switch (polData.GetPolarization(0)) {
        case aocommon::Polarization::XX:
          pol1 = 'X';
          pol2 = 'X';
          break;
        case aocommon::Polarization::XY:
          pol1 = 'X';
          pol2 = 'Y';
          break;
        case aocommon::Polarization::YX:
          pol1 = 'Y';
          pol2 = 'X';
          break;
        case aocommon::Polarization::YY:
          pol1 = 'Y';
          pol2 = 'Y';
          break;
        case aocommon::Polarization::LL:
          pol1 = 'L';
          pol2 = 'L';
          break;
        case aocommon::Polarization::LR:
          pol1 = 'L';
          pol2 = 'R';
          break;
        case aocommon::Polarization::RL:
          pol1 = 'R';
          pol2 = 'L';
          break;
        case aocommon::Polarization::RR:
          pol1 = 'R';
          pol2 = 'R';
          break;
        default:
          throw std::runtime_error("Can't apply a bandpass for polarization " +
                                   aocommon::Polarization::TypeToShortString(
                                       polData.GetPolarization(0)));
      }

      for (size_t imageIndex = 0; imageIndex != polData.ImageCount();
           ++imageIndex) {
        Image2DCPtr uncorrected = polData.GetImage(imageIndex);
        Image2DPtr newImage =
            apply(uncorrected, file, antenna1, antenna2, pol1, pol2);
        polData.SetImage(imageIndex, std::move(newImage));
      }
      data.SetPolarizationData(i, std::move(polData));
    }
  }

 private:
  static Image2DPtr apply(Image2DCPtr& uncorrected, BandpassFile& file,
                          const std::string& antenna1,
                          const std::string& antenna2, char pol1, char pol2) {
    Image2DPtr corrected =
        Image2D::MakePtr(uncorrected->Width(), uncorrected->Height());
    for (size_t ch = 0; ch != uncorrected->Height(); ++ch) {
      float val = 1.0 / (file.GetValue(antenna1, pol1, ch) *
                         file.GetValue(antenna2, pol2, ch));
      const num_t* ptrUncor = uncorrected->ValuePtr(0, ch);
      num_t* ptrCor = corrected->ValuePtr(0, ch);
      for (size_t x = 0; x != uncorrected->Width(); ++x) {
        ptrCor[x] = ptrUncor[x] * val;
      }
    }
    return corrected;
  }
};

}  // namespace algorithms

#endif
