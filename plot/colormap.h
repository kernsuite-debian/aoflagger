/** @file
 * This is the header file for the ColorMap base class and several specific
 * color map classes.
 * @author André Offringa <offringa@gmail.com>
 */
#ifndef COLORMAP_H
#define COLORMAP_H

#include <cmath>
#include <string>
#include <memory>

/**
 * The ColorMap class turns a value between -1 and 1 into a gradient color
 * scale.
 */
class ColorMap {
 public:
  enum Type {
    Grayscale,
    Inverted,
    HotCold,
    RedBlue,
    RedYellowBlue,
    Fire,
    Cool,
    BlackRed,
    CubeHelix,
    CubeHelixColourful,
    Viridis,
    Rainbow
  };
  /**
   * Destructor.
   */
  virtual ~ColorMap() {}
  /**
   * Maps a double value to the red component of the color map.
   * @param value Value to be mapped  (-1 to 1).
   * @return The red color value (0 - 255).
   */
  virtual unsigned char ValueToColorR(long double value) const noexcept = 0;
  /**
   * Maps a double value to the green component of the color map.
   * @param value Value to be mapped (-1 to 1).
   * @return The green color value (0 - 255).
   */
  virtual unsigned char ValueToColorG(long double value) const noexcept = 0;
  /**
   * Maps a double value to the blue component of the color map.
   * @param value Value to be mapped (-1 to 1).
   * @return The blue color value (0 - 255).
   */
  virtual unsigned char ValueToColorB(long double value) const noexcept = 0;
  /**
   * Maps a double value to the alfa (transparency) component of the color map.
   * @param value Value to be mapped (-1 to 1).
   * @return The alfa (transparency) color value (0 - 255). 255=fully opaque,
   * 0=fully transparent.
   */
  virtual unsigned char ValueToColorA(long double value) const noexcept = 0;

  /**
   * Convert the input value to a RGB value.
   * @param value Value to be mapped (-1 to 1).
   * @param r Red component (0-255)
   * @param g Green component (0-255)
   * @param b Blue component (0-255)
   */
  void Convert(double value, unsigned char& r, unsigned char& g,
               unsigned char& b) const noexcept {
    r = ValueToColorR(value);
    g = ValueToColorG(value);
    b = ValueToColorB(value);
  }

  /**
   * Create a color map given its type.
   * @param colorMapType one of the Type values.
   * @return The new created color map.
   */
  static std::unique_ptr<ColorMap> CreateColorMap(enum Type colorMapType);

  /**
   * Create a color map given its name. The human readable list of names can be
   * retrieved with GetColorMapsString().
   * @param typeStr name of the color map type.
   * @return The new create color map.
   * @see GetColorMapsString().
   */
  static std::unique_ptr<ColorMap> CreateColorMap(const std::string& typeStr);

  /**
   * Returns a string containing a description of the color map names. These
   * names can be used to create the color map with CreateColorMap().
   * @return
   * @see CreateColorMap().
   */
  static std::string GetColorMapsString();

 private:
  static const char* _colorMapsString;
};

/**
 * ColorMap that turns a value into a gray scale value. High values represent
 * whiter colors.
 */
class MonochromeMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    return (unsigned char)(value * 127.5 + 127.5);
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    return (unsigned char)(value * 127.5 + 127.5);
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    return (unsigned char)(value * 127.5 + 127.5);
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

/**
 * ColorMap that turns a value into a gray scale value. High values represent
 * whiter colors.
 */
class InvertedMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    return (unsigned char)(127.5 - value * 127.5);
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    return (unsigned char)(127.5 - value * 127.5);
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    return (unsigned char)(127.5 - value * 127.5);
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

/**
 * ColorMap that turns negative values into blue and positive values into red.
 * Zero is represented with black.
 */
class RedBlueMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    if (value > 0.0)
      return (unsigned char)(value * 255.0);
    else
      return 0;
  }
  unsigned char ValueToColorG(long double) const noexcept override { return 0; }
  unsigned char ValueToColorB(long double value) const noexcept override {
    if (value < 0.0)
      return (unsigned char)(value * -255.0);
    else
      return 0;
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

/**
 * ColorMap that turns negative values into red and positive values into black.
 * Zero is represented with white.
 */
class BlackRedMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    if (value > 0.0)
      return (unsigned char)(255 - value * 255.0);
    else
      return 255;
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    if (value > 0.0)
      return (unsigned char)(255 - value * 255.0);
    else
      return 255 + value * 255.0;
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    if (value > 0.0)
      return (unsigned char)(255 - value * 255.0);
    else
      return 255 + value * 255.0;
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

/**
 * ColorMap that turns negative values into blue and positive values into red.
 * Zero is represented with white.
 */
class RedWhiteBlueMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    if (value < 0.0)
      return (unsigned char)(255.0 + (value * 255.0));
    else
      return 255;
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    if (value < 0.0)
      return (unsigned char)(255.0 + (value * 255.0));
    else
      return (unsigned char)(255.0 - (value * 255.0));
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    if (value > 0.0)
      return (unsigned char)(255.0 - (value * 255.0));
    else
      return 255;
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

/**
 * ColorMap that passes several colors.
 */
class ColdHotMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    if (value >= 0.5)
      return (unsigned char)((1.5 - value) * 255.0);
    else if (value < -0.5)
      return 0;
    else if (value < 0.0)
      return (unsigned char)(((value + 0.5) * 2.0) * 255.0);
    else
      return 255;
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    if (value < -0.5)
      return (unsigned char)(((value + 1.0) * 2.0) * 255.0);
    else if (value < 0.0)
      return 255;
    else if (value < 0.5)
      return (unsigned char)(((0.5 - value) * 2.0) * 255.0);
    else
      return 0;
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    if (value < -0.5)
      return (unsigned char)((value + 1.5) * 255.0);
    else if (value < 0.0)
      return (unsigned char)(((-value) * 2.0) * 255.0);
    else
      return 0;
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

/**
 * ColorMap that uses another color map and increases its contrast.
 */
class ContrastMap : public ColorMap {
 private:
  const std::unique_ptr<ColorMap> _map;

 public:
  explicit ContrastMap(const std::string& type) : _map(CreateColorMap(type)) {}
  unsigned char ValueToColorR(long double value) const noexcept override {
    return _map->ValueToColorR(value >= 0.0 ? sqrt(value) : -sqrt(-value));
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    return _map->ValueToColorG(value >= 0.0 ? sqrt(value) : -sqrt(-value));
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    return _map->ValueToColorB(value >= 0.0 ? sqrt(value) : -sqrt(-value));
  }
  unsigned char ValueToColorA(long double value) const noexcept override {
    return _map->ValueToColorA(value >= 0.0 ? sqrt(value) : -sqrt(-value));
  }
};

/**
 * ColorMap that uses another color map and applies logarithmic scaling to it.
 * Expects inputs between 0 and 1, will output to map between -1 and 1.
 */
class PosLogMap : public ColorMap {
 private:
  const ColorMap& _map;

 public:
  explicit PosLogMap(const ColorMap& map) : _map(map) {}
  unsigned char ValueToColorR(long double value) const noexcept override {
    return value >= 0.0 ? _map.ValueToColorR(
                              (log10(value * 0.9999 + 0.0001) + 2.0) / 2.0)
                        : _map.ValueToColorR(-1.0);
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    return value >= 0.0 ? _map.ValueToColorG(
                              (log10(value * 0.9999 + 0.0001) + 2.0) / 2.0)
                        : _map.ValueToColorG(-1.0);
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    return value >= 0.0 ? _map.ValueToColorB(
                              (log10(value * 0.9999 + 0.0001) + 2.0) / 2.0)
                        : _map.ValueToColorB(-1.0);
  }
  unsigned char ValueToColorA(long double value) const noexcept override {
    return value >= 0.0 ? _map.ValueToColorA(
                              (log10(value * 0.9999 + 0.0001) + 2.0) / 2.0)
                        : _map.ValueToColorA(-1.0);
  }
};

class PosMonochromeLogMap : public PosLogMap {
 private:
  const MonochromeMap _monochromeMap;

 public:
  PosMonochromeLogMap() : PosLogMap(_monochromeMap) {}
};

/**
 * ColorMap that uses another color map and applies logarithmic scaling to it.
 * Expects inputs between -1 and 1, will output to map between -1 and 1.
 */
class FullLogMap : public ColorMap {
 private:
  const ColorMap& _map;

 public:
  explicit FullLogMap(const ColorMap& map) : _map(map) {}
  ~FullLogMap() {}
  unsigned char ValueToColorR(long double value) const noexcept override {
    return _map.ValueToColorR((log10(value * 0.49995 + 0.50005) + 2.0) / 2.0);
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    return _map.ValueToColorG((log10(value * 0.49995 + 0.50005) + 2.0) / 2.0);
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    return _map.ValueToColorB((log10(value * 0.49995 + 0.50005) + 2.0) / 2.0);
  }
  unsigned char ValueToColorA(long double value) const noexcept override {
    return _map.ValueToColorA((log10(value * 0.49995 + 0.50005) + 2.0) / 2.0);
  }
};

/**
 * ColorMap that turns negative values into blue and positive values into red.
 * Zero is represented with yellow.
 */
class RedYellowBlueMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    if (value >= 1.0 / 3.0)
      return 255;
    else if (value >= -1.0 / 3.0)
      return (unsigned char)(value * (255.0 * 3.0 / 2.0)) + 128;
    else
      return 0;
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    if (value >= 1.0 / 3.0)
      return 255 - (unsigned char)((value - 1.0 / 3.0) * (255.0 * 3.0 / 2.0));
    else if (value >= 0.0)
      return 255;
    else if (value >= -1.0 / 3.0)
      return (unsigned char)((value + 1.0 / 3.0) * (255.0 * 6.0 / 2.0));
    else
      return 0;
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    if (value >= 1.0 / 3.0)
      return 0;
    else if (value >= -1.0 / 3.0)
      return 255 - (unsigned char)((value + 1.0 / 3.0) * (255.0 * 3.0 / 2.0));
    else
      return (unsigned char)((value + 1.0) * (255.0 * 3.0 / 2.0));
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

class FireMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    if (value < -1.0)
      return 0;
    else if (value < 0)
      return (unsigned char)((value + 1.0) * 255.0);
    else
      return 255;
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    if (value < -0.5)
      return 0;
    else if (value < 0.5)
      return (unsigned char)((value + 0.5) * 255.0);
    else
      return 255;
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    if (value < 0)
      return 0;
    else if (value < 1.0)
      return (unsigned char)(value * 255.0);
    else
      return 255;
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

class CoolMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept final override {
    if (value < 0.0)
      return 0;
    else
      return (unsigned char)(value * 255.0);
  }
  unsigned char ValueToColorG(long double value) const noexcept final override {
    if (value < -0.5)
      return 0;
    else if (value > 0.5)
      return 255;
    else
      return (unsigned char)((value + 0.5) * 255.0);
  }
  unsigned char ValueToColorB(long double value) const noexcept final override {
    if (value < 0)
      return (value + 1.0) * 255.0;
    else
      return 255;
  }
  unsigned char ValueToColorA(long double) const noexcept final override {
    return 255;
  }
};

/**
 * ColorMap that turns negative values into blue and positive values into red.
 * Zero is represented with black.
 */
class RedYellowBlackBlueMap : public ColorMap {
 public:
  RedYellowBlackBlueMap() {}
  ~RedYellowBlackBlueMap() {}
  unsigned char ValueToColorR(long double value) const noexcept override {
    if (value < 0.0)
      return 0;
    else if (value < 0.5)
      return (unsigned char)(value * 2.0 * 255.0);
    else
      return 255;
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    if (value < -0.5)
      return (unsigned char)((value + 1.0) * 255.0 * 2.0);
    else if (value >= 0.5)
      return (unsigned char)((-value + 1.0) * 255.0 * 2.0);
    else if (value < 0.0)
      return (unsigned char)(-value * 2.0 * 255.0);
    else
      return (unsigned char)(value * 2.0 * 255.0);
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    if (value >= 0.0)
      return 0;
    else if (value >= -0.5)
      return (unsigned char)(-value * 2.0 * 255.0);
    else
      return 255;
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

/**
 * ColorMap that turns all negative values into black and positive values into a
 * gray scale.
 */
class PositiveMap : public ColorMap {
 public:
  PositiveMap() {}
  ~PositiveMap() {}
  unsigned char ValueToColorR(long double value) const noexcept override {
    return (unsigned char)(value > 0.0 ? value * 255.0 : 0);
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    return (unsigned char)(value > 0.0 ? value * 255.0 : 0);
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    return (unsigned char)(value > 0.0 ? value * 255.0 : 0);
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

/**
 * ColorMap that is the negation of PositiveMap.
 */
class InvPositiveMap : public ColorMap {
 public:
  InvPositiveMap() {}
  ~InvPositiveMap() {}
  unsigned char ValueToColorR(long double value) const noexcept override {
    return (unsigned char)(value > 0.0 ? 255.0 - value * 255.0 : 255.0);
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    return (unsigned char)(value > 0.0 ? 255.0 - value * 255.0 : 255.0);
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    return (unsigned char)(value > 0.0 ? 255.0 - value * 255.0 : 255.0);
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

/**
 * ColorMap that is the logarithmic negation of PositiveMap.
 */
class LogInvPositiveMap : public ColorMap {
 public:
  LogInvPositiveMap() {}
  ~LogInvPositiveMap() {}
  unsigned char ValueToColorR(long double value) const noexcept override {
    return (unsigned char)(value > 0.0 ? -log10(value * 0.9 + 0.1) * 255.0
                                       : 255.0);
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    return (unsigned char)(value > 0.0 ? -log10(value * 0.9 + 0.1) * 255.0
                                       : 255.0);
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    return (unsigned char)(value > 0.0 ? -log10(value * 0.9 + 0.1) * 255.0
                                       : 255.0);
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }
};

/**
 * ColorMap that turns different integer values into different colours.
 */
class IntMap {
 public:
  static unsigned char R(int value) {
    switch (value % 16) {
      case 0:
      case 3:
      case 5:
      case 6:
        return 255;
      case 8:
      case 11:
      case 13:
      case 14:
        return 127;
      default:
        return 0;
    }
  }
  static unsigned char G(int value) {
    switch (value % 16) {
      case 1:
      case 3:
      case 4:
      case 6:
        return 255;
      case 9:
      case 11:
      case 12:
      case 14:
        return 127;
      default:
        return 0;
    }
  }
  static unsigned char B(int value) {
    switch (value % 16) {
      case 2:
      case 4:
      case 5:
      case 6:
        return 255;
      case 10:
      case 12:
      case 13:
      case 14:
        return 127;
      default:
        return 0;
    }
  }
  static unsigned char A(int) { return 255; }
};

/**
 * ColorMap that is equivalent with the Viridis colormap as used by Python
 * Matplotlib.
 */
class ViridisMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    return (unsigned char)(DATA_R[index(value)] * 255.0);
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    return (unsigned char)(DATA_G[index(value)] * 255.0);
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    return (unsigned char)(DATA_B[index(value)] * 255.0);
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }

 private:
  static size_t index(long double value) {
    double d = round(value * 127.5 + 127.5);
    if (d <= 0.0) d = 0.0;
    if (d >= 255.0) d = 255.0;
    return (size_t)d;
  }

  static const double DATA_R[256], DATA_G[256], DATA_B[256];
};

template <int Saturation>
class CubeHelixMapBase : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    unsigned char r, g, b;
    convertColour(value, r, g, b);
    return r;
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    unsigned char r, g, b;
    convertColour(value, r, g, b);
    return g;
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    unsigned char r, g, b;
    convertColour(value, r, g, b);
    return b;
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }

 private:
  void convertColour(long double value, unsigned char& rv, unsigned char& gv,
                     unsigned char& bv) const noexcept {
    value = value * 0.5 + 0.5;
    double start = 0.5, cycles = -1.5, gamma = 1.5;
    double phi = 2. * 3.14 * (start / 3.0 + value * cycles);
    if (gamma != 1.0) value = pow(value, 1. / gamma);
    double a = (0.01 * Saturation) * value * (1. - value) / 2.;
    double r = value + a * (-0.14861 * cos(phi) + 1.78277 * sin(phi));
    double g = value + a * (-0.29227 * cos(phi) - 0.90649 * sin(phi));
    double b = value + a * (1.97294 * cos(phi));
    int ri = r * 256;
    int gi = g * 256;
    int bi = b * 256;
    if (ri > 255) ri = 255;
    if (ri < 0) ri = 0;
    if (gi > 255) gi = 255;
    if (gi < 0) gi = 0;
    if (bi > 255) bi = 255;
    if (bi < 0) bi = 0;
    rv = (unsigned char)ri;
    gv = (unsigned char)gi;
    bv = (unsigned char)bi;
  }
};

class CubeHelixMap : public CubeHelixMapBase<100> {};
class CubeHelixColourfulMap : public CubeHelixMapBase<150> {};

class RainbowMap : public ColorMap {
 public:
  unsigned char ValueToColorR(long double value) const noexcept override {
    long double r, g, b;
    scaledWavelengthToRGB(value, r, g, b);
    return static_cast<unsigned char>(std::min<double>(r * 256.0, 255.0));
  }
  unsigned char ValueToColorG(long double value) const noexcept override {
    long double r, g, b;
    scaledWavelengthToRGB(value, r, g, b);
    return static_cast<unsigned char>(std::min<double>(g * 256.0, 255.0));
  }
  unsigned char ValueToColorB(long double value) const noexcept override {
    long double r, g, b;
    scaledWavelengthToRGB(value, r, g, b);
    return static_cast<unsigned char>(std::min<double>(b * 256.0, 255.0));
  }
  unsigned char ValueToColorA(long double) const noexcept override {
    return 255;
  }

 private:
  static void wavelengthToRGB(long double wavelength, long double& red,
                              long double& green, long double& blue) {
    if (wavelength >= 350.0 && wavelength < 440.0) {
      red = -(wavelength - 440.0) / (440.0 - 350.0);
      green = 0.0;
      blue = 1.0;
    } else if (wavelength >= 440.0 && wavelength < 490.0) {
      red = 0.0;
      green = (wavelength - 440.0) / (490.0 - 440.0);
      blue = 1.0;
    } else if (wavelength >= 490.0 && wavelength < 510.0) {
      red = 0.0;
      green = 1.0;
      blue = -(wavelength - 510.0) / (510.0 - 490.0);
    } else if (wavelength >= 510.0 && wavelength < 580.0) {
      red = (wavelength - 510.0) / (580.0 - 510.0);
      green = 1.0;
      blue = 0.0;
    } else if (wavelength >= 580.0 && wavelength < 645.0) {
      red = 1.0;
      green = -(wavelength - 645.0) / (645.0 - 580.0);
      blue = 0.0;
    } else {
      // (wavelength >= 645.0 && wavelength < 780.0) is red, but in other cases
      // also use red:
      red = 1.0;
      green = 0.0;
      blue = 0.0;
    }
    if (wavelength >= 350.0 && wavelength < 420.0) {
      long double factor;
      factor = 0.3 + 0.7 * (wavelength - 350.0) / (420.0 - 350.0);
      red *= factor;
      green *= factor;
      blue *= factor;
    } else if (wavelength >= 420.0 && wavelength <= 700.0) {
      // nothing to be done
    } else if (wavelength > 700.0 && wavelength <= 780.0) {
      long double factor;
      factor = 0.3 + 0.7 * (780.0 - wavelength) / (780.0 - 700.0);
      red *= factor;
      green *= factor;
      blue *= factor;
    } else if (wavelength > 780.0) {
      long double factor;
      factor = 0.3;
      red *= factor;
      green *= factor;
      blue *= factor;
    } else {
      red = 0.0;
      green = 0.0;
      blue = 0.0;
    }
  }

  static void scaledWavelengthToRGB(long double position, long double& red,
                                    long double& green, long double& blue) {
    wavelengthToRGB(((position + 1.0) * 0.5) * 300.0 + 400.0, red, green, blue);
    if (red < 0.0) red = 0.0;
    if (red > 1.0) red = 1.0;
    if (green < 0.0) green = 0.0;
    if (green > 1.0) green = 1.0;
    if (blue < 0.0) blue = 0.0;
    if (blue > 1.0) blue = 1.0;
  }
};

#endif
