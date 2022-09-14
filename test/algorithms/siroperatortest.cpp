#include "../../structures/mask2d.h"

#include "../../algorithms/siroperator.h"

#include "../../util/rng.h"

#include <boost/test/unit_test.hpp>

#include <string>
#include <sstream>

using algorithms::SIROperator;

BOOST_AUTO_TEST_SUITE(sir_operator_test, *boost::unit_test::label("algorithms"))

static std::string flagsToString(const bool* flags, unsigned size) {
  std::stringstream s;
  for (unsigned i = 0; i < size; ++i) {
    s << (flags[i] ? 'x' : ' ');
  }
  return s.str();
}

static void setFlags(bool* flags, const std::string& str) {
  unsigned index = 0;
  for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
    flags[index] = ((*i) == 'x');
    ++index;
  }
}

static std::string maskToString(Mask2D& mask) {
  std::stringstream s;
  for (unsigned y = 0; y < mask.Height(); ++y) {
    for (unsigned x = 0; x < mask.Width(); ++x) {
      s << (mask.Value(x, y) ? 'x' : ' ');
    }
  }
  return s.str();
}

static void setMask(Mask2D& mask, const std::string& str) {
  std::string::const_iterator i = str.begin();
  for (unsigned y = 0; y < mask.Height(); ++y) {
    for (unsigned x = 0; x < mask.Width(); ++x) {
      mask.SetValue(x, y, (*i) == 'x');
      ++i;
    }
  }
}

template <typename Functor>
inline void TestImplementation(Functor operate) {
  bool* flags = new bool[40];
  setFlags(flags, "     x    ");

  operate(flags, 10, 0.0);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "     x    ");

  operate(flags, 10, 0.4);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "     x    ");

  operate(flags, 10, 0.5);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "    xxx   ");

  operate(flags, 10, 0.0);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "    xxx   ");

  operate(flags, 10, 0.25);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "   xxxxx  ");

  operate(flags, 10, 0.16);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "   xxxxx  ");

  operate(flags, 10, 0.17);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "  xxxxxxx ");

  operate(flags, 10, 1.0);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "xxxxxxxxxx");

  setFlags(flags, "xx xx     ");

  operate(flags, 10, 0.0);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "xx xx     ");

  operate(flags, 10, 0.19);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "xx xx     ");

  setFlags(flags, "xx xx     ");
  operate(flags, 10, 0.2);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "xxxxx     ");

  setFlags(flags, "x         ");
  operate(flags, 10, 0.5);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "xx        ");
  operate(flags, 10, 0.4);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "xxx       ");

  setFlags(flags, "         x");
  operate(flags, 10, 0.5);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "        xx");
  operate(flags, 10, 0.4);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "       xxx");

  setFlags(flags, " x        ");
  operate(flags, 10, 0.4);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), " x        ");

  setFlags(flags, "        x ");
  operate(flags, 10, 0.4);
  BOOST_CHECK_EQUAL(flagsToString(flags, 10), "        x ");

  //               0    5    0    5    0    5    0    5
  setFlags(flags, "     xxxxxx xx xx x x xxx xxxxx         ");
  operate(flags, 40, 0.2);
  BOOST_CHECK_EQUAL(flagsToString(flags, 40),
                    "    xxxxxxxxxxxxx x xxxxxxxxxxxx        ");

  setFlags(flags, "     xxxxxx xx xx x x xxx xxxxx         ");
  operate(flags, 40, 0.3);
  BOOST_CHECK_EQUAL(flagsToString(flags, 40),
                    "   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx       ");

  setFlags(flags, "     xxxxxx xx xx x x xxx xxxxx         ");
  operate(flags, 40, 0.4);
  BOOST_CHECK_EQUAL(flagsToString(flags, 40),
                    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  ");

  setFlags(flags, "xxxxxxxxxxxxxxx       xxxxxxxxxxxxxxxxxx");
  operate(flags, 40, 0.3);
  BOOST_CHECK_EQUAL(flagsToString(flags, 40),
                    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

  setFlags(flags, "      x   x  x xx xxx    ");
  operate(flags, 25, 0.5);
  BOOST_CHECK_EQUAL(flagsToString(flags, 25), "     xxxxxxxxxxxxxxxxxxxx");

  delete[] flags;
}

BOOST_AUTO_TEST_CASE(three_pass_algorithm) {
  TestImplementation(SIROperator::Operate);
}

BOOST_AUTO_TEST_CASE(speed, *boost::unit_test::disabled()) {
  const unsigned flagsSize = 100000;
  bool flags[flagsSize];
  for (unsigned i = 0; i < flagsSize; ++i) {
    flags[i] = (RNG::Uniform() >= 0.2);
  }
  SIROperator::Operate(flags, flagsSize, 0.1);
}

BOOST_AUTO_TEST_CASE(time_direction) {
  Mask2D mask = Mask2D::MakeSetMask<false>(10, 1);
  setMask(mask, "     x    ");

  SIROperator::OperateHorizontally(mask, 0.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");

  SIROperator::OperateHorizontally(mask, 0.4);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");

  SIROperator::OperateHorizontally(mask, 0.5);
  BOOST_CHECK_EQUAL(maskToString(mask), "    xxx   ");

  SIROperator::OperateHorizontally(mask, 0.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "    xxx   ");

  SIROperator::OperateHorizontally(mask, 0.25);
  BOOST_CHECK_EQUAL(maskToString(mask), "   xxxxx  ");

  SIROperator::OperateHorizontally(mask, 0.16);
  BOOST_CHECK_EQUAL(maskToString(mask), "   xxxxx  ");

  SIROperator::OperateHorizontally(mask, 0.17);
  BOOST_CHECK_EQUAL(maskToString(mask), "  xxxxxxx ");

  SIROperator::OperateHorizontally(mask, 0.6);
  BOOST_CHECK_EQUAL(maskToString(mask), "xxxxxxxxxx");

  SIROperator::OperateHorizontally(mask, 1.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "xxxxxxxxxx");

  setMask(mask, "xx xx     ");

  SIROperator::OperateHorizontally(mask, 0.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "xx xx     ");

  SIROperator::OperateHorizontally(mask, 0.19);
  BOOST_CHECK_EQUAL(maskToString(mask), "xx xx     ");

  SIROperator::OperateHorizontally(mask, 0.2);
  BOOST_CHECK_EQUAL(maskToString(mask), "xxxxx     ");

  mask = Mask2D::MakeSetMask<false>(40, 1);
  //             0    5    0    5    0    5    0    5
  setMask(mask, "     xxxxxx xx xx x x xxx xxxxx         ");
  SIROperator::OperateHorizontally(mask, 0.2);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "    xxxxxxxxxxxxx x xxxxxxxxxxxx        ");

  setMask(mask, "     xxxxxx xx xx x x xxx xxxxx         ");
  SIROperator::OperateHorizontally(mask, 0.3);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx       ");

  setMask(mask, "     xxxxxx xx xx x x xxx xxxxx         ");
  SIROperator::OperateHorizontally(mask, 0.4);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  ");

  setMask(mask, "xxxxxxxxxxxxxxx       xxxxxxxxxxxxxxxxxx");
  SIROperator::OperateHorizontally(mask, 0.3);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
}

BOOST_AUTO_TEST_CASE(masked_time_direction) {
  Mask2D mask = Mask2D::MakeSetMask<false>(10, 1);
  Mask2D missingA = Mask2D::MakeSetMask<false>(10, 1);
  Mask2D missingB = Mask2D::MakeSetMask<false>(10, 1);
  setMask(mask, "     x    ");
  setMask(missingA, "          ");
  setMask(missingB, "    xxx   ");

  SIROperator::OperateHorizontallyMissing(mask, missingA, 0.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");
  SIROperator::OperateHorizontallyMissing(mask, missingB, 0.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");

  SIROperator::OperateHorizontallyMissing(mask, missingA, 0.4);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");
  SIROperator::OperateHorizontallyMissing(mask, missingB, 0.4);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");

  SIROperator::OperateHorizontallyMissing(mask, missingB, 0.5);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");
  SIROperator::OperateHorizontallyMissing(mask, missingA, 0.5);
  BOOST_CHECK_EQUAL(maskToString(mask), "    xxx   ");

  SIROperator::OperateHorizontallyMissing(mask, missingB, 0.25);
  BOOST_CHECK_EQUAL(maskToString(mask), "    xxx   ");
  SIROperator::OperateHorizontallyMissing(mask, missingA, 0.25);
  BOOST_CHECK_EQUAL(maskToString(mask), "   xxxxx  ");

  setMask(mask, "   x   xx ");
  SIROperator::OperateHorizontallyMissing(mask, missingB, 0.25);
  BOOST_CHECK_EQUAL(maskToString(mask), "  xx   xxx");
  setMask(mask, "   x   xx ");
  SIROperator::OperateHorizontallyMissing(mask, missingA, 0.25);
  BOOST_CHECK_EQUAL(maskToString(mask), "   x   xx ");

  mask = Mask2D::MakeSetMask<false>(46, 1);
  missingA = Mask2D::MakeSetMask<false>(46, 1);
  //                 0    5    0    5    0    5    0    5    0    5
  setMask(mask, "     xx  xxxx xx xxx x  x x xxx xxxxx         ");
  setMask(missingA, "       xx          x   x   xx                 ");
  SIROperator::OperateHorizontallyMissing(mask, missingA, 0.2);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "    xxx  xxxxxxxxxxx x  xxx xxxxxxxxxx        ");

  setMask(mask, "     xx  xxxx xx xx  x  x x  xx xxxxx         ");
  setMask(missingA, "       xx          x   x   xx                 ");
  SIROperator::OperateHorizontallyMissing(mask, missingA, 0.3);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "   xxxx  xxxxxxxxxx xxx xxx  xxxxxxxxxx       ");

  setMask(mask, "x x x      xxxxxx xx xx x x xxx xxxxx         ");
  setMask(missingA, "xxxxxx                                        ");
  SIROperator::OperateHorizontallyMissing(mask, missingA, 0.4);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "x x x xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  ");

  setMask(mask, " xxxxxxxxxxxxxxx       x xxxxxxxxxxxxxxxxx x x");
  setMask(missingA, "x                       x                 xxxx");
  SIROperator::OperateHorizontallyMissing(mask, missingA, 0.3);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    " xxxxxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxx x x");
}

BOOST_AUTO_TEST_CASE(frequency_direction) {
  Mask2D mask = Mask2D::MakeSetMask<false>(1, 10);
  setMask(mask, "     x    ");

  SIROperator::OperateVertically(mask, 0.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");

  SIROperator::OperateVertically(mask, 0.4);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");

  SIROperator::OperateVertically(mask, 0.5);
  BOOST_CHECK_EQUAL(maskToString(mask), "    xxx   ");

  SIROperator::OperateVertically(mask, 0.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "    xxx   ");

  SIROperator::OperateVertically(mask, 0.25);
  BOOST_CHECK_EQUAL(maskToString(mask), "   xxxxx  ");

  SIROperator::OperateVertically(mask, 0.16);
  BOOST_CHECK_EQUAL(maskToString(mask), "   xxxxx  ");

  SIROperator::OperateVertically(mask, 0.17);
  BOOST_CHECK_EQUAL(maskToString(mask), "  xxxxxxx ");

  SIROperator::OperateVertically(mask, 0.6);
  BOOST_CHECK_EQUAL(maskToString(mask), "xxxxxxxxxx");

  SIROperator::OperateVertically(mask, 1.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "xxxxxxxxxx");

  setMask(mask, "xx xx     ");

  SIROperator::OperateVertically(mask, 0.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "xx xx     ");

  SIROperator::OperateVertically(mask, 0.19);
  BOOST_CHECK_EQUAL(maskToString(mask), "xx xx     ");

  SIROperator::OperateVertically(mask, 0.2);
  BOOST_CHECK_EQUAL(maskToString(mask), "xxxxx     ");

  mask = Mask2D::MakeSetMask<false>(1, 40);
  //             0    5    0    5    0    5    0    5
  setMask(mask, "     xxxxxx xx xx x x xxx xxxxx         ");
  SIROperator::OperateVertically(mask, 0.2);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "    xxxxxxxxxxxxx x xxxxxxxxxxxx        ");

  setMask(mask, "     xxxxxx xx xx x x xxx xxxxx         ");
  SIROperator::OperateVertically(mask, 0.3);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx       ");

  setMask(mask, "     xxxxxx xx xx x x xxx xxxxx         ");
  SIROperator::OperateVertically(mask, 0.4);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  ");

  setMask(mask, "xxxxxxxxxxxxxxx       xxxxxxxxxxxxxxxxxx");
  SIROperator::OperateVertically(mask, 0.3);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
}

BOOST_AUTO_TEST_CASE(masked_frequency_direction) {
  Mask2D mask = Mask2D::MakeSetMask<false>(1, 10);
  Mask2D missingA = Mask2D::MakeSetMask<false>(1, 10);
  Mask2D missingB = Mask2D::MakeSetMask<false>(1, 10);
  setMask(mask, "     x    ");
  setMask(missingA, "          ");
  setMask(missingB, "    xxx   ");

  SIROperator::OperateVerticallyMissing(mask, missingA, 0.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");
  SIROperator::OperateVerticallyMissing(mask, missingB, 0.0);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");

  SIROperator::OperateVerticallyMissing(mask, missingA, 0.4);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");
  SIROperator::OperateVerticallyMissing(mask, missingB, 0.4);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");

  SIROperator::OperateVerticallyMissing(mask, missingB, 0.5);
  BOOST_CHECK_EQUAL(maskToString(mask), "     x    ");
  SIROperator::OperateVerticallyMissing(mask, missingA, 0.5);
  BOOST_CHECK_EQUAL(maskToString(mask), "    xxx   ");

  SIROperator::OperateVerticallyMissing(mask, missingB, 0.25);
  BOOST_CHECK_EQUAL(maskToString(mask), "    xxx   ");
  SIROperator::OperateVerticallyMissing(mask, missingA, 0.25);
  BOOST_CHECK_EQUAL(maskToString(mask), "   xxxxx  ");

  setMask(mask, "   x   xx ");
  SIROperator::OperateVerticallyMissing(mask, missingB, 0.25);
  BOOST_CHECK_EQUAL(maskToString(mask), "  xx   xxx");
  setMask(mask, "   x   xx ");
  SIROperator::OperateVerticallyMissing(mask, missingA, 0.25);
  BOOST_CHECK_EQUAL(maskToString(mask), "   x   xx ");

  mask = Mask2D::MakeSetMask<false>(1, 46);
  missingA = Mask2D::MakeSetMask<false>(1, 46);
  //                 0    5    0    5    0    5    0    5    0    5
  setMask(mask, "     xx  xxxx xx xxx x  x x xxx xxxxx         ");
  setMask(missingA, "       xx          x   x   xx                 ");
  SIROperator::OperateVerticallyMissing(mask, missingA, 0.2);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "    xxx  xxxxxxxxxxx x  xxx xxxxxxxxxx        ");

  setMask(mask, "     xx  xxxx xx xx  x  x x  xx xxxxx         ");
  setMask(missingA, "       xx          x   x   xx                 ");
  SIROperator::OperateVerticallyMissing(mask, missingA, 0.3);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "   xxxx  xxxxxxxxxx xxx xxx  xxxxxxxxxx       ");

  setMask(mask, "x x x      xxxxxx xx xx x x xxx xxxxx         ");
  setMask(missingA, "xxxxxx                                        ");
  SIROperator::OperateVerticallyMissing(mask, missingA, 0.4);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    "x x x xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  ");

  setMask(mask, " xxxxxxxxxxxxxxx       x xxxxxxxxxxxxxxxxx x x");
  setMask(missingA, "x                       x                 xxxx");
  SIROperator::OperateVerticallyMissing(mask, missingA, 0.3);
  BOOST_CHECK_EQUAL(maskToString(mask),
                    " xxxxxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxx x x");
}

BOOST_AUTO_TEST_CASE(time_direction_speed, *boost::unit_test::disabled()) {
  const unsigned flagsSize = 10000;
  const unsigned channels = 256;
  Mask2D mask = Mask2D::MakeSetMask<false>(flagsSize, channels);
  for (unsigned y = 0; y < channels; ++y) {
    for (unsigned i = 0; i < flagsSize; ++i) {
      mask.SetValue(i, 0, (RNG::Uniform() >= 0.2));
    }
  }
  SIROperator::OperateHorizontally(mask, 0.1);
}

BOOST_AUTO_TEST_SUITE_END()
