#ifndef BANDPASS_FILE_H
#define BANDPASS_FILE_H

#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

#include "../../util/logger.h"

class BandpassFile {
 public:
  BandpassFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file)
      throw std::runtime_error("Can not find bandpass file: '" + filename +
                               '\'');
    std::string antenna, pol;
    size_t channel;
    double value;
    while (file) {
      file >> antenna >> pol >> channel >> value;
      if (file.good()) {
        char polChar = pol[0];
        _values.emplace(BandpassIndex{antenna, polChar, channel}, value);
        // Logger::Info << antenna << " , " << polChar << " , " << channel <<
        // '\n';
      }
    }
    Logger::Info << "Read " << _values.size() << " passband values from file "
                 << filename << ".\n";
  }

  double GetValue(const std::string& antenna, char polarization,
                  size_t channel) const {
    auto iter = _values.find(BandpassIndex{antenna, polarization, channel});
    if (iter == _values.end()) {
      throw std::runtime_error("Passband file is missing values for " +
                               antenna + " pol " + polarization + " ch " +
                               std::to_string(channel));
    }
    return iter->second;
  }

 private:
  struct BandpassIndex {
    std::string antenna;
    char polarization;
    size_t channel;

    bool operator<(const BandpassIndex& rhs) const {
      if (channel < rhs.channel)
        return true;
      else if (channel == rhs.channel) {
        if (polarization < rhs.polarization)
          return true;
        else if (polarization == rhs.polarization && antenna < rhs.antenna)
          return true;
      }
      return false;
    }
  };

  /** antenna, polarization, */
  std::map<BandpassIndex, double> _values;
};

#endif
