#include "../../structures/antennainfo.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(antenna_info, *boost::unit_test::label("structures"))

static AntennaInfo Create(unsigned id, double x, double y, double z,
                          const std::string& name, double diameter,
                          const std::string& mount,
                          const std::string& station) {
  AntennaInfo antenna;
  antenna.id = id;
  antenna.position.x = x;
  antenna.position.y = y;
  antenna.position.z = z;
  antenna.name = name;
  antenna.diameter = diameter;
  antenna.mount = mount;
  antenna.station = station;
  return antenna;
}

BOOST_AUTO_TEST_CASE(equality_operator) {
  {
    const AntennaInfo lhs = Create(1, 2, 3, 4, "name", 5, "mount", "station");
    BOOST_CHECK(lhs == lhs);  // Test self-comparison.
    {                         // Finite values can be compared.
      {
        AntennaInfo rhs = Create(42, 2, 3, 4, "name", 5, "mount", "station");
        BOOST_CHECK(!(lhs == rhs));
      }
      {
        AntennaInfo rhs = Create(1, 42, 3, 4, "name", 5, "mount", "station");
        BOOST_CHECK(!(lhs == rhs));
      }
      {
        AntennaInfo rhs = Create(1, 2, 42, 4, "name", 5, "mount", "station");
        BOOST_CHECK(!(lhs == rhs));
      }
      {
        AntennaInfo rhs = Create(1, 2, 3, 42, "name", 5, "mount", "station");
        BOOST_CHECK(!(lhs == rhs));
      }
      {
        AntennaInfo rhs = Create(1, 2, 3, 4, "Name", 5, "mount", "station");
        BOOST_CHECK(!(lhs == rhs));
      }
      {
        AntennaInfo rhs = Create(1, 2, 3, 4, "name", 42, "mount", "station");
        BOOST_CHECK(!(lhs == rhs));
      }
      {
        AntennaInfo rhs = Create(1, 2, 3, 4, "name", 5, "Mount", "station");
        BOOST_CHECK(!(lhs == rhs));
      }
      {
        AntennaInfo rhs = Create(1, 2, 3, 4, "name", 5, "mount", "Station");
        BOOST_CHECK(!(lhs == rhs));
      }
    }
  }
  {  // Infinite values can be compared.
     // The unit test of position already tests infinity.
    const double infinity = std::numeric_limits<double>::infinity();
    AntennaInfo lhs_and_rhs =
        Create(1, 2, 3, 4, "name", infinity, "mount", "station");
    BOOST_CHECK(lhs_and_rhs == lhs_and_rhs);
    lhs_and_rhs = Create(1, 2, 3, 4, "name", -infinity, "mount", "station");
    BOOST_CHECK(lhs_and_rhs == lhs_and_rhs);
  }
  {  // NaN values are never equal.
     // The unit test of position already tests NaN.
    double nan = std::numeric_limits<double>::quiet_NaN();
    AntennaInfo lhs_and_rhs =
        Create(1, 2, 3, 4, "name", nan, "mount", "station");
    BOOST_CHECK(!(lhs_and_rhs == lhs_and_rhs));

    nan = std::copysign(nan, -1);
    lhs_and_rhs = Create(1, 2, 3, 4, "name", nan, "mount", "station");
    BOOST_CHECK(!(lhs_and_rhs == lhs_and_rhs));
  }
}

BOOST_AUTO_TEST_SUITE_END()
