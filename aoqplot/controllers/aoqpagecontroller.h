#ifndef AOQ_PAGE_CONTROLLER_H
#define AOQ_PAGE_CONTROLLER_H

#include "../../structures/antennainfo.h"

#include <vector>

class AOQPageController {
 public:
  virtual ~AOQPageController() {}

  virtual void SetStatistics(const class StatisticsCollection* statCollection,
                             const std::vector<AntennaInfo>& antennas) {}

  virtual void SetHistograms(const class HistogramCollection* histograms) {}

  virtual void CloseStatistics() = 0;
};

#endif
