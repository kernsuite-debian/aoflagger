#ifndef ANTENNAINFO_H
#define ANTENNAINFO_H

#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "types.h"

#include "../util/serializable.h"

class EarthPosition {
 public:
  EarthPosition() : x(0.0), y(0.0), z(0.0) {}

  double x, y, z;
  std::string ToString() {
    std::stringstream s;
    s.setf(std::ios::fixed, std::ios::floatfield);
    s.width(16);
    s.precision(16);
    s << x << "," << y << "," << z << " (alt " << sqrtl(x * x + y * y + z * z)
      << "), or "
      << "N" << Latitude() * 180 / M_PI << " E" << Longitude() * 180 / M_PI;
    return s.str();
  }

  double Longitude() const { return atan2l(y, x); }

  long double LongitudeL() const { return atan2l(y, x); }

  double Latitude() const {
    return atan2l(z, sqrtl((long double)x * x + y * y));
  }

  long double LatitudeL() const {
    return atan2l(z, sqrtl((long double)x * x + y * y));
  }

  double Altitude() const { return sqrtl((long double)x * x + y * y + z * z); }

  double AltitudeL() const { return sqrtl((long double)x * x + y * y + z * z); }

  void Serialize(std::ostream& stream) const {
    Serializable::SerializeToDouble(stream, x);
    Serializable::SerializeToDouble(stream, y);
    Serializable::SerializeToDouble(stream, z);
  }

  void Unserialize(std::istream& stream) {
    x = Serializable::UnserializeDouble(stream);
    y = Serializable::UnserializeDouble(stream);
    z = Serializable::UnserializeDouble(stream);
  }

  double Distance(const EarthPosition& other) const {
    return sqrt(DistanceSquared(other));
  }

  double DistanceSquared(const EarthPosition& other) const {
    double dx = x - other.x, dy = y - other.y, dz = z - other.z;
    return dx * dx + dy * dy + dz * dz;
  }

  friend bool operator==(const EarthPosition& lhs, const EarthPosition& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
  }
};

class UVW {
 public:
  UVW() : u(0.0), v(0.0), w(0.0) {}
  UVW(num_t _u, num_t _v, num_t _w) : u(_u), v(_v), w(_w) {}
  num_t u, v, w;
};

class AntennaInfo {
 public:
  AntennaInfo() {}

  unsigned id;
  EarthPosition position;
  std::string name;
  double diameter;
  std::string mount;
  std::string station;

  void Serialize(std::ostream& stream) const {
    Serializable::SerializeToUInt32(stream, id);
    position.Serialize(stream);
    Serializable::SerializeToString(stream, name);
    Serializable::SerializeToDouble(stream, diameter);
    Serializable::SerializeToString(stream, mount);
    Serializable::SerializeToString(stream, station);
  }

  void Unserialize(std::istream& stream) {
    id = Serializable::UnserializeUInt32(stream);
    position.Unserialize(stream);
    Serializable::UnserializeString(stream, name);
    diameter = Serializable::UnserializeDouble(stream);
    Serializable::UnserializeString(stream, mount);
    Serializable::UnserializeString(stream, station);
  }

  friend bool operator==(const AntennaInfo& lhs, const AntennaInfo& rhs) {
    return lhs.id == rhs.id && lhs.position == rhs.position &&
           lhs.name == rhs.name && lhs.diameter == rhs.diameter &&
           lhs.mount == rhs.mount && lhs.station == rhs.station;
  }
};

class ChannelInfo {
 public:
  unsigned frequencyIndex;
  double frequencyHz;
  double channelWidthHz;
  double effectiveBandWidthHz;
  double resolutionHz;

  double MetersToLambda(double meters) const {
    return meters * frequencyHz / 299792458.0L;
  }
  void Serialize(std::ostream& stream) const {
    Serializable::SerializeToUInt32(stream, frequencyIndex);
    Serializable::SerializeToDouble(stream, frequencyHz);
    Serializable::SerializeToDouble(stream, channelWidthHz);
    Serializable::SerializeToDouble(stream, effectiveBandWidthHz);
    Serializable::SerializeToDouble(stream, resolutionHz);
  }

  void Unserialize(std::istream& stream) {
    frequencyIndex = Serializable::UnserializeUInt32(stream);
    frequencyHz = Serializable::UnserializeDouble(stream);
    channelWidthHz = Serializable::UnserializeDouble(stream);
    effectiveBandWidthHz = Serializable::UnserializeDouble(stream);
    resolutionHz = Serializable::UnserializeDouble(stream);
  }
};

class BandInfo {
 public:
  unsigned windowIndex;
  std::vector<ChannelInfo> channels;

  BandInfo() : windowIndex(0) {}

  num_t CenterFrequencyHz() const {
    num_t total = 0.0;
    for (std::vector<ChannelInfo>::const_iterator i = channels.begin();
         i != channels.end(); ++i)
      total += i->frequencyHz;
    return total / channels.size();
  }
  void Serialize(std::ostream& stream) const {
    Serializable::SerializeToUInt32(stream, windowIndex);
    Serializable::SerializeToUInt32(stream, channels.size());
    for (std::vector<ChannelInfo>::const_iterator i = channels.begin();
         i != channels.end(); ++i)
      i->Serialize(stream);
  }

  void Unserialize(std::istream& stream) {
    windowIndex = Serializable::UnserializeUInt32(stream);
    size_t channelCount = Serializable::UnserializeUInt32(stream);
    channels.resize(channelCount);
    for (size_t i = 0; i < channelCount; ++i) channels[i].Unserialize(stream);
  }

  std::pair<size_t, size_t> GetChannelRange(double startFrequencyHz,
                                            double endFrequencyHz) const {
    size_t first = channels.size(), last = 0;
    for (size_t ch = 0; ch != channels.size(); ++ch) {
      if (channels[ch].frequencyHz >= startFrequencyHz &&
          channels[ch].frequencyHz < endFrequencyHz) {
        first = std::min(first, ch);
        last = std::max(last, ch);
      }
    }
    if (first == channels.size())
      return std::make_pair(0u, 0u);
    else
      return std::make_pair(first, last + 1);
  }
};

class FieldInfo {
 public:
  FieldInfo() {}
  FieldInfo(const FieldInfo& source)
      : fieldId(source.fieldId),
        delayDirectionRA(source.delayDirectionRA),
        delayDirectionDec(source.delayDirectionDec),
        name(source.name) {}
  FieldInfo& operator=(const FieldInfo& source) {
    fieldId = source.fieldId;
    delayDirectionRA = source.delayDirectionRA;
    delayDirectionDec = source.delayDirectionDec;
    name = source.name;
    return *this;
  }

  friend bool operator==(const FieldInfo& lhs, const FieldInfo& rhs) {
    return lhs.fieldId == rhs.fieldId &&
           lhs.delayDirectionRA == rhs.delayDirectionRA &&
           lhs.delayDirectionDec == rhs.delayDirectionDec &&
           lhs.name == rhs.name;
  }

  unsigned fieldId;
  num_t delayDirectionRA;
  num_t delayDirectionDec;
  std::string name;
};

class Baseline {
 public:
  EarthPosition antenna1, antenna2;
  Baseline() : antenna1(), antenna2() {}
  Baseline(const AntennaInfo& _antenna1, const AntennaInfo& _antenna2)
      : antenna1(_antenna1.position), antenna2(_antenna2.position) {}
  Baseline(const EarthPosition& _antenna1, const EarthPosition& _antenna2)
      : antenna1(_antenna1), antenna2(_antenna2) {}

  num_t Distance() const {
    num_t dx = antenna1.x - antenna2.x;
    num_t dy = antenna1.y - antenna2.y;
    num_t dz = antenna1.z - antenna2.z;
    return sqrtn(dx * dx + dy * dy + dz * dz);
  }
  num_t Angle() const {
    num_t dz = antenna1.z - antenna2.z;
    // baseline is either orthogonal to the earths axis, or
    // the length of the baseline is zero.
    if (dz == 0.0) return 0.0;
    num_t transf = 1.0 / (antenna1.z - antenna2.z);
    num_t dx = (antenna1.x - antenna2.x) * transf;
    num_t dy = (antenna1.y - antenna2.y) * transf;
    num_t length = sqrtn(dx * dx + dy * dy + 1.0);
    return acosn(1.0 / length);
  }
  num_t DeltaX() const { return antenna2.x - antenna1.x; }
  num_t DeltaY() const { return antenna2.y - antenna1.y; }
  num_t DeltaZ() const { return antenna2.z - antenna1.z; }
};

class Frequency {
 public:
  static std::string ToString(num_t value) {
    std::stringstream s;
    if (fabs(value) >= 1000000000.0L)
      s << round(value / 10000000.0L) / 100.0L << " GHz";
    else if (fabs(value) >= 1000000.0L)
      s << round(value / 10000.0L) / 100.0L << " MHz";
    else if (fabs(value) >= 1000.0L)
      s << round(value / 10.0L) / 100.0L << " KHz";
    else
      s << value << " Hz";
    return s.str();
  }
};

class RightAscension {
 public:
  static std::string ToString(numl_t value) {
    value = fmod(value, 2.0 * M_PInl);
    if (value < 0.0) value += 2.0 * M_PInl;
    std::stringstream s;
    s << (int)floorn(value * 12.0 / M_PInl) << ':';
    int d2 = (int)floornl(fmodnl(value * 12.0 * 60.0 / M_PInl, 60.0));
    if (d2 < 10) s << '0';
    s << d2 << ':';
    numl_t d3 = fmodnl(value * 12.0 * 60.0 * 60.0 / M_PInl, 60.0);
    if (d3 < 10.0) s << '0';
    s << d3;
    return s.str();
  }
};

class Declination {
 public:
  static std::string ToString(numl_t value) {
    value = fmod(value, 2.0 * M_PInl);
    if (value < 0.0) value += 2.0 * M_PInl;
    if (value > M_PInl * 0.5) value = M_PInl - value;
    std::stringstream s;
    if (value > 0.0)
      s << '+';
    else
      s << '-';
    value = fabsnl(value);
    s << (int)floornl(value * 180.0 / M_PIn) << '.';
    int d2 = (int)fmodnl(value * 180.0 * 60.0 / M_PIn, 60.0);
    if (d2 < 10) s << '0';
    s << d2 << '.';
    numl_t d3 = fmodnl(value * 180.0 * 60.0 * 60.0 / M_PIn, 60.0);
    if (d3 < 10.0) s << '0';
    s << d3;
    return s.str();
  }
};

class Angle {
 public:
  static std::string ToString(numl_t valueRad) {
    std::stringstream s;
    numl_t deg = valueRad * 180.0 / M_PI;
    if (std::abs(deg) > 3)
      s << deg << " deg";
    else if (std::abs(deg) > 3.0 / 60.0)
      s << (deg / 60.0) << " arcmin";
    else
      s << (deg / 3600.0) << " arcsec";
    return s.str();
  }
};

#endif
