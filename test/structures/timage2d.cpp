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

BOOST_AUTO_TEST_CASE(construct_from_initializer_list) {
  const Image2D empty(0, 0, {});
  BOOST_CHECK_EQUAL(empty.Width(), 0);
  BOOST_CHECK_EQUAL(empty.Height(), 0);

  constexpr size_t kWidth = 2;
  constexpr size_t kHeight = 2;
  const Image2D from_init_list(kWidth, kHeight, {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  BOOST_CHECK_EQUAL(from_init_list.Width(), kWidth);
  BOOST_CHECK_GE(from_init_list.Stride(), kWidth);
  BOOST_CHECK_EQUAL(from_init_list.Height(), kHeight);
  for (size_t i = 0; i != kWidth * kHeight; ++i) {
    BOOST_CHECK_EQUAL(from_init_list.Value(i % kWidth, i / kWidth), i + 1);
  }
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

BOOST_AUTO_TEST_CASE(operator_equals) {
  Image2D a = Image2D::MakeZeroImage(10, 10);
  Image2D b = Image2D::MakeZeroImage(10, 10);
  Image2D c = Image2D::MakeZeroImage(10, 10);
  Image2D d = Image2D::MakeZeroImage(8, 10);
  c.SetValue(5, 5, 1.0);
  BOOST_CHECK(a == a);
  BOOST_CHECK(b == b);
  BOOST_CHECK(c == c);
  BOOST_CHECK(d == d);
  BOOST_CHECK(a == b);
  BOOST_CHECK(!(a == c));
  BOOST_CHECK(!(b == c));
  BOOST_CHECK(!(a == d));
  BOOST_CHECK(!(c == d));
}

BOOST_AUTO_TEST_CASE(all_finite) {
  Image2D image = Image2D::MakeZeroImage(10, 10);
  BOOST_CHECK(image.AllFinite());
  image.SetValue(9, 9, std::numeric_limits<num_t>::quiet_NaN());
  BOOST_CHECK(!image.AllFinite());
  image.SetValue(9, 9, std::numeric_limits<num_t>::infinity());
  BOOST_CHECK(!image.AllFinite());
  image.SetValue(9, 9, -std::numeric_limits<num_t>::infinity());
  BOOST_CHECK(!image.AllFinite());
}

BOOST_AUTO_TEST_CASE(make_finite_copy) {
  Image2D image = Image2D::MakeUnsetImage(10, 10);
  for (size_t y = 0; y != image.Height(); ++y) {
    for (size_t x = 0; x != image.Width(); ++x) {
      image.SetValue(x, y, x + y * image.Height());
    }
  }

  BOOST_CHECK(image.MakeFiniteCopy() == image);

  Image2D reference(image);
  reference.SetValue(4, 6, 0.0);

  image.SetValue(4, 6, std::numeric_limits<num_t>::quiet_NaN());
  BOOST_CHECK(image.MakeFiniteCopy() == reference);
  image.SetValue(4, 6, std::numeric_limits<num_t>::infinity());
  BOOST_CHECK(image.MakeFiniteCopy() == reference);
  image.SetValue(4, 6, -std::numeric_limits<num_t>::infinity());
  BOOST_CHECK(image.MakeFiniteCopy() == reference);
}

BOOST_AUTO_TEST_SUITE_END()
