#ifndef VERSION_STRING_H
#define VERSION_STRING_H

#include <stdexcept>
#include <string>

class VersionString {
 public:
  VersionString()
      : _major(0), _minor(0), _subminor(0), _hasMinor(0), _hasSubminor(0) {}

  VersionString(const std::string& str) {
    if (str.empty()) throw std::runtime_error("Empty version string specified");
    for (size_t i = 0; i != str.size(); ++i)
      if (str[i] != '.' && (str[i] < '0' || str[i] > '9'))
        throw std::runtime_error("Invalid version specified: '" + str + "'");
    const size_t pos1 = str.find('.');
    if (pos1 == std::string::npos) {
      _major = std::atoi(str.c_str());
      _minor = 0;
      _subminor = 0;
      _hasMinor = false;
      _hasSubminor = false;
    } else {
      _major = std::atoi(str.substr(0, pos1).c_str());
      _hasMinor = true;
      const size_t pos2 = str.find('.', pos1 + 1);
      if (pos2 == std::string::npos) {
        _minor = std::atoi(str.substr(pos1 + 1).c_str());
        _subminor = 0;
        _hasSubminor = false;
      } else {
        _minor = std::atoi(str.substr(pos1 + 1, pos2 - pos1).c_str());
        _subminor = std::atoi(str.substr(pos2 + 1).c_str());
        _hasSubminor = true;
      }
    }
  }

  int Major() const { return _major; }
  int Minor() const { return _minor; }
  int Subminor() const { return _subminor; }
  bool HasMinor() const { return _hasMinor; }
  bool HasSubminor() const { return _hasSubminor; }

  std::string String() const {
    if (_hasMinor) {
      if (_hasSubminor) {
        return std::to_string(_major) + '.' + std::to_string(_minor) + '.' +
               std::to_string(_subminor);
      } else {
        return std::to_string(_major) + '.' + std::to_string(_minor);
      }
    } else
      return std::to_string(_major);
  }

 private:
  int _major, _minor, _subminor;
  bool _hasMinor, _hasSubminor;
};

#endif
