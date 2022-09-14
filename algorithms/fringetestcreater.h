#ifndef FRINGETESTCREATER_H
#define FRINGETESTCREATER_H

#include "../structures/timefrequencymetadata.h"

namespace algorithms {

class FringeTestCreater {
 public:
  static void AddStaticFringe(class TimeFrequencyData& ftData,
                              TimeFrequencyMetaDataCPtr metaData,
                              long double strength);

 private:
  FringeTestCreater() {}
  ~FringeTestCreater() {}
};

}  // namespace algorithms

#endif
