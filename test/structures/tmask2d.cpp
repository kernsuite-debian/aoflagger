#include "../../structures/mask2d.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(mask2d, *boost::unit_test::label("structures"))

BOOST_AUTO_TEST_CASE(to_string) {
  Mask2D a = Mask2D::MakeSetMask<false>(2, 3);
  BOOST_CHECK_EQUAL(a.ToString(),
                    "  \n"
                    "  \n"
                    "  \n");

  Mask2D b = Mask2D::MakeSetMask<true>(2, 3);
  BOOST_CHECK_EQUAL(b.ToString(),
                    "XX\n"
                    "XX\n"
                    "XX\n");
  Mask2D c = Mask2D::MakeSetMask<false>(2, 3);
  c.SetValue(0, 0, true);
  c.SetValue(1, 1, true);
  c.SetValue(1, 2, true);
  BOOST_CHECK_EQUAL(c.ToString(),
                    "X \n"
                    " X\n"
                    " X\n");
}

BOOST_AUTO_TEST_SUITE_END()
