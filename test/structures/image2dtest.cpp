#include "../../structures/image2d.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(image2d, *boost::unit_test::label("structures"))

static void AssertAll(const Image2D& image, float value) {
  for (size_t y = 0; y != image.Height(); ++y) {
    for (size_t x = 0; x != image.Width(); ++x) {
      BOOST_CHECK_EQUAL(image.Value(x, y), value);
      BOOST_CHECK_EQUAL(image.ValuePtr(0, y)[x], value);
    }
  }
}

BOOST_AUTO_TEST_CASE(construction) {
  Image2D image(Image2D::MakeSetImage(10, 10, 1.0));
  AssertAll(image, 1.0);

  Image2D copy(image);
  AssertAll(copy, 1.0);

  Image2D moved(std::move(copy));
  AssertAll(moved, 1.0);

  Image2D unset(Image2D::MakeUnsetImage(5, 5));
  unset.SetValue(1, 2, 3.0);
  BOOST_CHECK_EQUAL(unset.Value(1, 2), 3.0);
}

BOOST_AUTO_TEST_CASE(assignment) {
  Image2D image1 = Image2D::MakeSetImage(10, 10, 1.0);
  AssertAll(image1, 1.0);

  Image2D image2 = Image2D::MakeSetImage(10, 10, 2.0);
  AssertAll(image2, 2.0);

  image1 = image2;
  AssertAll(image1, 2.0);
  AssertAll(image2, 2.0);

  Image2D image3 = Image2D::MakeSetImage(10, 10, 3.0);
  AssertAll(image3, 3.0);

  image1 = std::move(image3);
  AssertAll(image1, 3.0);

  Image2D unset(Image2D::MakeUnsetImage(5, 5));
  unset = std::move(image3);
  AssertAll(image1, 3.0);
}

BOOST_AUTO_TEST_SUITE_END()
