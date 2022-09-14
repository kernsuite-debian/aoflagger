#include "../../structures/antennainfo.h"

#include <boost/test/unit_test.hpp>

#include <sstream>

BOOST_AUTO_TEST_SUITE(field_info, *boost::unit_test::label("structures"))

static FieldInfo Create(unsigned id, num_t ra, num_t dec,
                        const std::string& name) {
  FieldInfo field;
  field.fieldId = id;
  field.delayDirectionRA = ra;
  field.delayDirectionDec = dec;
  field.name = name;
  return field;
}

BOOST_AUTO_TEST_CASE(equality_operator) {
  {
    const FieldInfo lhs = Create(1, 2, 3, "name");
    BOOST_CHECK(lhs == lhs);  // Test self-comparison.
    {                         // Finite values can be compared.
      FieldInfo rhs = Create(42, 2, 3, "name");
      BOOST_CHECK(!(lhs == rhs));
    }
    {
      FieldInfo rhs = Create(1, 42, 3, "name");
      BOOST_CHECK(!(lhs == rhs));
    }
    {
      FieldInfo rhs = Create(1, 2, 42, "name");
      BOOST_CHECK(!(lhs == rhs));
    }
    {
      FieldInfo rhs = Create(1, 2, 3, "Name");
      BOOST_CHECK(!(lhs == rhs));
    }
  }
  {  // Infinite values can be compared.
    const double infinity = std::numeric_limits<double>::infinity();
    FieldInfo lhs_and_rhs = Create(1, infinity, -infinity, "name");
    BOOST_CHECK(lhs_and_rhs == lhs_and_rhs);
  }
  {  // NaN values are never equal.
    double nan = std::numeric_limits<num_t>::quiet_NaN();
    FieldInfo lhs_and_rhs = Create(1, nan, 3, "name");
    BOOST_CHECK(!(lhs_and_rhs == lhs_and_rhs));
    lhs_and_rhs = Create(1, 2, nan, "name");
    BOOST_CHECK(!(lhs_and_rhs == lhs_and_rhs));

    nan = std::copysign(nan, -1);
    lhs_and_rhs = Create(1, nan, 3, "name");
    BOOST_CHECK(!(lhs_and_rhs == lhs_and_rhs));
    lhs_and_rhs = Create(1, 2, nan, "name");
    BOOST_CHECK(!(lhs_and_rhs == lhs_and_rhs));
  }
}

BOOST_AUTO_TEST_SUITE_END()
