#include "integerdomain.h"

#include <string>

IntegerDomain::IntegerDomain(const IntegerDomain& source) {
  for (std::vector<int>::const_iterator i = source._values.begin();
       i != source._values.end(); ++i)
    _values.push_back(*i);
}

IntegerDomain::IntegerDomain(int singleValue) {
  _values.push_back(singleValue);
}

IntegerDomain::IntegerDomain(const int* values, unsigned count) {
  for (unsigned i = 0; i < count; ++i) _values.push_back(values[i]);
}

IntegerDomain::IntegerDomain(int first, unsigned count) {
  for (int i = first; i < (int)(first + count); ++i) _values.push_back(i);
}

IntegerDomain::IntegerDomain(int first, unsigned step, unsigned count) {
  for (unsigned i = 0; i < count; ++i) _values.push_back(first + i * step);
}

IntegerDomain::IntegerDomain(const std::string& str) {
  enum State { Accepting, Number, Interval } state = Accepting;
  int number, i1 = 0;
  for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
    switch (state) {
      case Accepting:
        if (*i >= '0' && *i <= '9') {
          state = Number;
          number = *i - '0';
        }
        break;
      case Number:
        if (*i >= '0' && *i <= '9') {
          number = number * 10 + (*i - '0');
        } else if (*i == '-') {
          state = Interval;
          i1 = number;
          number = 0;
        }
        break;
      case Interval:
        if (*i >= '0' && *i <= '9') {
          number = number * 10 + (*i - '0');
        }
        break;
    }
  }
  switch (state) {
    case Number:
      _values.push_back(number);
      break;
    case Interval:
      for (int n = i1; n <= number; ++n) _values.push_back(n);
      break;
    case Accepting:
      break;
  }
}

void IntegerDomain::Join(const IntegerDomain& other) throw() {
  for (std::vector<int>::const_iterator i = other._values.begin();
       i != other._values.end(); ++i) {
    bool found = false;
    for (std::vector<int>::const_iterator j = _values.begin();
         j != _values.end(); ++j) {
      if (*i == *j) {
        found = true;
        break;
      }
    }
    if (!found) _values.push_back(*i);
  }
}

IntegerDomain IntegerDomain::Split(unsigned partCount,
                                   unsigned partIndex) const {
  const unsigned start = _values.size() * partIndex / partCount;
  const unsigned end = _values.size() * (partIndex + 1) / partCount;

  int* values = new int[end - start];
  for (unsigned i = start; i < end; ++i) values[i - start] = _values[i];
  const IntegerDomain domain(*values, end - start);
  delete[] values;
  return domain;
}
