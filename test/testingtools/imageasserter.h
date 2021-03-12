#ifndef AOFLAGGER_IMAGE_ASSERTER_H
#define AOFLAGGER_IMAGE_ASSERTER_H

#include "../../structures/image2d.h"

#include <boost/test/unit_test.hpp>

#include <string>
#include <stdexcept>
#include <sstream>

class ImageAsserter {
 public:
  static void AssertEqual(const Image2DCPtr &actual,
                          const Image2DCPtr &expected) {
    BOOST_CHECK_EQUAL(actual->Width(), expected->Width());
    BOOST_CHECK_EQUAL(actual->Height(), expected->Height());

    for (size_t y = 0; y < actual->Height(); ++y) {
      for (size_t x = 0; x < actual->Width(); ++x) {
        BOOST_CHECK_CLOSE(actual->Value(x, y), expected->Value(x, y), 1e-3);
      }
    }
  }

  static void AssertConstant(const Image2DCPtr &actual, const num_t &expected) {
    for (size_t y = 0; y < actual->Height(); ++y) {
      for (size_t x = 0; x < actual->Width(); ++x) {
        BOOST_CHECK_CLOSE(actual->Value(x, y), expected, 1e-3);
      }
    }
  }

  static void AssertFinite(const Image2DCPtr &actual) {
    size_t nonFinites = 0;
    for (size_t y = 0; y < actual->Height(); ++y) {
      for (size_t x = 0; x < actual->Width(); ++x) {
        if (!std::isfinite(actual->Value(x, y))) {
          ++nonFinites;
        }
      }
    }
    BOOST_CHECK_EQUAL(nonFinites, 0);
  }
};

#endif
