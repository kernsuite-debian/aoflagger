#include "indexableset.h"

namespace rfiStrategy {

bool IndexableSet::FindExtremeBaseline(rfiStrategy::ImageSet* imageSet,
                                       rfiStrategy::ImageSetIndex& index,
                                       bool longest) {
  rfiStrategy::IndexableSet* iSet =
      dynamic_cast<rfiStrategy::IndexableSet*>(imageSet);
  if (iSet != nullptr) {
    double extremeSq = longest ? 0.0 : std::numeric_limits<double>::max();
    size_t antCount = iSet->AntennaCount();
    std::vector<AntennaInfo> antennas(antCount);
    for (size_t a = 0; a != antCount; a++)
      antennas[a] = iSet->GetAntennaInfo(a);

    rfiStrategy::ImageSetIndex loopIndex(iSet->StartIndex());
    if (loopIndex.HasWrapped()) return false;
    const size_t band = iSet->GetBand(index),
                 sequenceId = iSet->GetSequenceId(index);
    while (!loopIndex.HasWrapped()) {
      if (sequenceId == iSet->GetSequenceId(loopIndex) &&
          band == iSet->GetBand(loopIndex)) {
        size_t a1 = iSet->GetAntenna1(loopIndex);
        size_t a2 = iSet->GetAntenna2(loopIndex);
        const AntennaInfo &ant1 = antennas[a1], &ant2 = antennas[a2];
        double distSq = ant1.position.DistanceSquared(ant2.position);
        if (longest) {
          if (distSq > extremeSq) {
            extremeSq = distSq;
            index = loopIndex;
          }
        } else {
          if (distSq < extremeSq && a1 != a2) {
            extremeSq = distSq;
            index = loopIndex;
          }
        }
      }
      loopIndex.Next();
    }
    if (!longest && extremeSq == std::numeric_limits<double>::max())
      return false;
    else
      return true;
  }
  return false;
}

bool IndexableSet::FindMedianBaseline(rfiStrategy::ImageSet* imageSet,
                                      rfiStrategy::ImageSetIndex& index) {
  rfiStrategy::IndexableSet* iSet =
      dynamic_cast<rfiStrategy::IndexableSet*>(imageSet);
  if (iSet == nullptr) {
    return false;
  } else {
    size_t antCount = iSet->AntennaCount();
    std::vector<AntennaInfo> antennas(antCount);
    for (size_t a = 0; a != antCount; a++)
      antennas[a] = iSet->GetAntennaInfo(a);

    rfiStrategy::ImageSetIndex loopIndex(iSet->StartIndex());
    const size_t band = iSet->GetBand(index),
                 sequenceId = iSet->GetSequenceId(index);
    std::vector<std::pair<double, rfiStrategy::ImageSetIndex>> distances;
    while (!loopIndex.HasWrapped()) {
      if (sequenceId == iSet->GetSequenceId(loopIndex) &&
          band == iSet->GetBand(loopIndex)) {
        size_t a1 = iSet->GetAntenna1(loopIndex);
        size_t a2 = iSet->GetAntenna2(loopIndex);
        const AntennaInfo &ant1 = antennas[a1], &ant2 = antennas[a2];
        distances.emplace_back(ant1.position.DistanceSquared(ant2.position),
                               loopIndex);
      }
      loopIndex.Next();
    }
    size_t n = distances.size();
    if (n == 0)
      return false;
    else {
      std::nth_element(
          distances.begin(), distances.begin() + n / 2, distances.end(),
          [](const std::pair<double, rfiStrategy::ImageSetIndex>& l,
             const std::pair<double, rfiStrategy::ImageSetIndex>& r) {
            return l.first < r.first;
          });
      index = (distances.begin() + n / 2)->second;
      return true;
    }
  }
}

}  // namespace rfiStrategy
