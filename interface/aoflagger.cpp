#include "aoflagger.h"

#include "../lua/telescopefile.h"

#include <version.h>

namespace aoflagger {

std::string AOFlagger::FindStrategyFile(enum TelescopeId telescopeId,
                                        const std::string& scenario) {
  return TelescopeFile::FindStrategy(
      "", static_cast<TelescopeFile::TelescopeId>(telescopeId), scenario);
}

QualityStatistics AOFlagger::MakeQualityStatistics(
    const double* scanTimes, size_t nScans, const double* channelFrequencies,
    size_t nChannels, size_t nPolarizations) {
  return QualityStatistics(scanTimes, nScans, channelFrequencies, nChannels,
                           nPolarizations, false);
}

QualityStatistics AOFlagger::MakeQualityStatistics(
    const double* scanTimes, size_t nScans, const double* channelFrequencies,
    size_t nChannels, size_t nPolarizations, bool computeHistograms) {
  return QualityStatistics(scanTimes, nScans, channelFrequencies, nChannels,
                           nPolarizations, computeHistograms);
}

std::string AOFlagger::GetVersionString() { return AOFLAGGER_VERSION_STR; }

void AOFlagger::GetVersion(short& major, short& minor, short& subMinor) {
  major = AOFLAGGER_VERSION_MAJOR;
  minor = AOFLAGGER_VERSION_MINOR;
  subMinor = AOFLAGGER_VERSION_SUBMINOR;
}

std::string AOFlagger::GetVersionDate() { return AOFLAGGER_VERSION_DATE_STR; }

}  // end of namespace aoflagger
