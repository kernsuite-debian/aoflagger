#include "../../structures/antennainfo.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(earth_position, *boost::unit_test::label("structures"))

static EarthPosition Create(double x, double y, double z) {
  EarthPosition position;
  position.x = x;
  position.y = y;
  position.z = z;
  return position;
}

BOOST_AUTO_TEST_CASE(equality_operator) {
  {  // Finite values can be compared.
    const EarthPosition lhs = Create(1, 2, 3);
    BOOST_CHECK(lhs == lhs);  // Test self-comparison.
    {
      EarthPosition rhs = Create(42, 2, 3);
      BOOST_CHECK(!(lhs == rhs));
    }
    {
      EarthPosition rhs = Create(1, 42, 3);
      BOOST_CHECK(!(lhs == rhs));
    }
    {
      EarthPosition rhs = Create(1, 2, 42);
      BOOST_CHECK(!(lhs == rhs));
    }
  }
  {  // Infinite values can be compared.
    const double infinity = std::numeric_limits<double>::infinity();
    EarthPosition lhs_and_rhs = Create(infinity, 0, -infinity);
    BOOST_CHECK(lhs_and_rhs == lhs_and_rhs);
  }
  {  // NaN values are never equal.
    double nan = std::numeric_limits<double>::quiet_NaN();
    EarthPosition lhs_and_rhs = Create(nan, 0, 0);
    BOOST_CHECK(!(lhs_and_rhs == lhs_and_rhs));

    nan = std::copysign(nan, -1);
    lhs_and_rhs = Create(nan, 0, 0);
    BOOST_CHECK(!(lhs_and_rhs == lhs_and_rhs));
  }
}

BOOST_AUTO_TEST_SUITE_END()
