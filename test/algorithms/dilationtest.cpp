#include "../../structures/mask2d.h"

#include "../../algorithms/morphologicalflagger.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(dilation, *boost::unit_test::label("algorithms"))

static std::string maskToString(Mask2DCPtr mask, bool flip) {
  std::stringstream s;
  if (flip) {
    for (unsigned x = 0; x < mask->Width(); ++x) {
      for (unsigned y = 0; y < mask->Height(); ++y)
        s << (mask->Value(x, y) ? 'x' : ' ');
      s << '\n';
    }
  } else {
    for (unsigned y = 0; y < mask->Height(); ++y) {
      for (unsigned x = 0; x < mask->Width(); ++x)
        s << (mask->Value(x, y) ? 'x' : ' ');
      s << '\n';
    }
  }
  return s.str();
}

static void setMask(Mask2DPtr mask, bool flip, const std::string &str) {
  std::string::const_iterator i = str.begin();
  if (flip) {
    for (unsigned x = 0; x < mask->Width(); ++x) {
      for (unsigned y = 0; y < mask->Height(); ++y) {
        mask->SetValue(x, y, (*i) == 'x');
        ++i;
      }
      ++i;  // newline
    }
  } else {
    for (unsigned y = 0; y < mask->Height(); ++y) {
      for (unsigned x = 0; x < mask->Width(); ++x) {
        mask->SetValue(x, y, (*i) == 'x');
        ++i;
      }
      ++i;  // newline
    }
  }
}

template <bool Flip, typename DilateFunction>
static void testDilation(DilateFunction dilate) {
  std::string expected;
  Mask2DPtr mask = Mask2D::CreateSetMaskPtr<false>(5, 5);
  dilate(mask.get(), 0);
  expected =
      "     \n"
      "     \n"
      "     \n"
      "     \n"
      "     \n";
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);
  dilate(mask.get(), 1);
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);
  dilate(mask.get(), 2);
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);
  dilate(mask.get(), 3);
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);
  dilate(mask.get(), 4);
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);
  dilate(mask.get(), 5);
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);
  dilate(mask.get(), 6);
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);

  expected =
      "     \n"
      "     \n"
      "  x  \n"
      "     \n"
      "     \n";
  setMask(mask, Flip, expected);
  dilate(mask.get(), 0);
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);

  dilate(mask.get(), 1);
  expected =
      "     \n"
      "     \n"
      " xxx \n"
      "     \n"
      "     \n";
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);

  dilate(mask.get(), 1);
  expected =
      "     \n"
      "     \n"
      "xxxxx\n"
      "     \n"
      "     \n";
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);
  dilate(mask.get(), 1);
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);
  dilate(mask.get(), 100);
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);

  setMask(mask, Flip,
          " x   \n"
          "   x \n"
          "x    \n"
          "    x\n"
          " x x \n");
  dilate(mask.get(), 2);
  expected =
      "xxxx \n"
      " xxxx\n"
      "xxx  \n"
      "  xxx\n"
      "xxxxx\n";
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);

  setMask(mask, Flip,
          "x    \n"
          " x   \n"
          "  x  \n"
          "   x \n"
          "    x\n");
  dilate(mask.get(), 6);
  expected =
      "xxxxx\n"
      "xxxxx\n"
      "xxxxx\n"
      "xxxxx\n"
      "xxxxx\n";
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);

  if (Flip)
    mask = Mask2D::CreateSetMaskPtr<false>(4, 6);
  else
    mask = Mask2D::CreateSetMaskPtr<false>(6, 4);
  dilate(mask.get(), 0);
  expected =
      "      \n"
      "      \n"
      "      \n"
      "      \n";
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);

  setMask(mask, Flip,
          "x     \n"
          "  x   \n"
          "    x \n"
          "     x\n");
  dilate(mask.get(), 4);
  expected =
      "xxxxx \n"
      "xxxxxx\n"
      "xxxxxx\n"
      " xxxxx\n";
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);

  setMask(mask, Flip,
          "x     \n"
          "      \n"
          "    x \n"
          "     x\n");
  dilate(mask.get(), 7);
  expected =
      "xxxxxx\n"
      "      \n"
      "xxxxxx\n"
      "xxxxxx\n";
  BOOST_CHECK_EQUAL(maskToString(mask, Flip), expected);
}

BOOST_AUTO_TEST_CASE(horizontal_dilation) {
  testDilation<false>(MorphologicalFlagger::DilateFlagsHorizontally);
}

BOOST_AUTO_TEST_CASE(vertical_dilation) {
  testDilation<true>(MorphologicalFlagger::DilateFlagsVertically);
}

BOOST_AUTO_TEST_SUITE_END()
