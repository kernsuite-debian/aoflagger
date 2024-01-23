#ifndef RESTORE_CHANNEL_RANGE_H
#define RESTORE_CHANNEL_RANGE_H

#include <utility>
#include <vector>

#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

namespace algorithms {

class RestoreChannelRange {
 public:
  static void Execute(TimeFrequencyData& lhs, const TimeFrequencyData& rhs,
                      const TimeFrequencyMetaData& metaData, double startMHz,
                      double endMHz) {
    size_t maskCount = lhs.MaskCount();
    std::vector<Mask2DPtr> cMasks(maskCount);
    for (size_t m = 0; m != maskCount; ++m)
      cMasks[m] = Mask2D::MakePtr(*lhs.GetMask(m));

    std::vector<Mask2DCPtr> originalMasks(maskCount);
    if (rhs.MaskCount() == 1 && maskCount != 1) {
      for (size_t m = 0; m != maskCount; ++m) originalMasks[m] = rhs.GetMask(0);
    } else {
      for (size_t m = 0; m != maskCount; ++m) originalMasks[m] = rhs.GetMask(m);
    }

    const std::vector<ChannelInfo>& band = metaData.Band().channels;
    for (size_t ch = 0; ch != band.size(); ++ch) {
      if (band[ch].frequencyHz >= startMHz * 1e6 &&
          band[ch].frequencyHz <= endMHz * 1e6) {
        for (size_t m = 0; m != maskCount; ++m) {
          for (size_t x = 0; x != originalMasks[m]->Width(); ++x)
            cMasks[m]->SetValue(x, ch, originalMasks[m]->Value(x, ch));
        }
      }
    }

    for (size_t m = 0; m != maskCount; ++m)
      lhs.SetMask(m, std::move(cMasks[m]));
  }
};

}  // namespace algorithms

#endif
