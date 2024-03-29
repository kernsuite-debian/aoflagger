#include <cmath>
#include <stdexcept>
#include <string>

#ifndef HAVE_EXP10
#define exp10(x) exp((2.3025850929940456840179914546844) * (x))
#endif

class NumberParsingException : public std::runtime_error {
 public:
  explicit NumberParsingException(const std::string& str)
      : std::runtime_error(std::string("Failed to parse number: ") + str) {}
};

/**
 * This class implements functions that copy the behaviour of atof and strtod,
 * etc. Unfortunately, atof are locale-dependent and e.g. a Dutch machine will
 * interpret a decimal point differently. This is undesirable, hence the
 * reimplementation here.
 * TODO can be replaced by std::from_chars() once C++17 is used.
 */
class NumberParser {
 public:
  /**
   * @throws NumberParsingException when @c str can not be parsed.
   */
  static double ToDouble(const char* str) { return toNumber<double>(str); }

  /**
   * @throws NumberParsingException when @c str can not be parsed.
   */
  static short ToShort(const char* str) { return toSignedInteger<short>(str); }

 private:
  /**
   * @throws NumberParsingException when @c str can not be parsed.
   */
  template <typename T>
  static T toNumber(const char* str) {
    if (*str == 0) throw NumberParsingException("Supplied string is empty");

    // Read optional sign
    bool isNegative;
    if (isNegativeSymbol(*str)) {
      isNegative = true;
      ++str;
    } else if (isPositiveSymbol(*str)) {
      isNegative = false;
      ++str;
    } else {
      isNegative = false;
    }

    // Read everything before decimal point
    if (!isDigit(*str))
      throw NumberParsingException(
          "Number did not start with digits after optional sign symbol");
    T val = (T)((*str) - '0');
    ++str;

    while (isDigit(*str)) {
      val = val * 10.0 + (T)((*str) - '0');
      ++str;
    }

    // Skip decimal point if given
    if (isDecimalPoint(*str)) ++str;

    // Read decimals
    T currentValue = 0.1;
    while (isDigit(*str)) {
      val += currentValue * (T)((*str) - '0');
      currentValue /= 10.0;
      ++str;
    }

    // Read optional exponent
    if (isExponentSymbol(*str)) {
      ++str;

      try {
        short e = ToShort(str);
        val = val * intPow10(e);
      } catch (NumberParsingException& e) {
        throw NumberParsingException("Could not parse exponent");
      }
    }
    // If there was an exponent, ToShort has checked for garbage at the end and
    // str is not updated, otherwise we still need to check that...
    else if (*str != 0)
      throw NumberParsingException(
          "The number contains invalid characters after its digits");

    if (isNegative)
      return -val;
    else
      return val;
  }

  /**
   * @throws NumberParsingException when @c str can not be parsed.
   */
  template <typename T>
  static T toSignedInteger(const char* str) {
    if (*str == 0) throw NumberParsingException("Supplied string is empty");

    // Read optional sign
    bool isNegative;
    if (isNegativeSymbol(*str)) {
      isNegative = true;
      ++str;
    } else if (isPositiveSymbol(*str)) {
      isNegative = false;
      ++str;
    } else {
      isNegative = false;
    }

    // Read digits
    if (!isDigit(*str))
      throw NumberParsingException(
          "Integer did not start with digits after optional sign symbol");
    T val = (T)((*str) - '0');
    ++str;

    while (isDigit(*str)) {
      val = val * 10 + (T)((*str) - '0');
      ++str;
    }

    // Check if str is completely read.
    if (*str == 0) {
      if (isNegative)
        return -val;
      else
        return val;
    } else {
      throw NumberParsingException(
          "The integer contains invalid characters after its digits");
    }
  }

  static double intPow10(int par) {
    // TODO this can be done a lot faster and more accurate with knowledge that
    // par = int.
    return exp10((double)par);
  }

  static bool isDigit(char c) { return c >= '0' && c <= '9'; }

  static bool isPositiveSymbol(char c) { return c == '+'; }

  static bool isNegativeSymbol(char c) { return c == '-'; }

  static bool isDecimalPoint(char c) { return c == '.'; }

  static bool isExponentSymbol(char c) { return c == 'e' || c == 'E'; }
};
