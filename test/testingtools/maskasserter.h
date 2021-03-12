#ifndef AOFLAGGER_MASK_ASSERTER_H
#define AOFLAGGER_MASK_ASSERTER_H

#include <string>
#include <stdexcept>
#include <sstream>

#include "../../structures/mask2d.h"

#include <boost/test/unit_test.hpp>

class MaskAsserter {
 public:
  static void AssertEqualMasks(const Mask2D& actual, const Mask2D& expected,
                               const std::string& str) {
    BOOST_CHECK_EQUAL(actual.Width(), expected.Width());
    BOOST_CHECK_EQUAL(actual.Height(), expected.Height());

    size_t errCount = 0;
    for (size_t y = 0; y < actual.Height(); ++y) {
      for (size_t x = 0; x < actual.Width(); ++x) {
        if (actual.Value(x, y) != expected.Value(x, y)) {
          ++errCount;
        }
      }
    }
    BOOST_CHECK_EQUAL(errCount, 0);
  }
};

#endif
