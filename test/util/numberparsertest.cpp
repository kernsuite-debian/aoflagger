#include "../../util/numberparser.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(number_parser, *boost::unit_test::label("util"))

BOOST_AUTO_TEST_CASE(to_double) {
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("1"), 1.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("1."), 1.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("1.000000"), 1.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("-1"), -1.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("-1.00000"), -1.0);

  BOOST_CHECK_CLOSE(NumberParser::ToDouble("3.14159265"), 3.14159265, 1e-5);
  BOOST_CHECK_CLOSE(NumberParser::ToDouble("0.00002"), 0.00002, 1e-5);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("234567"), 234567.0);
  BOOST_CHECK_CLOSE(NumberParser::ToDouble("234.567"), 234.567, 1e-5);

  BOOST_CHECK_EQUAL(NumberParser::ToDouble("0"), 0.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("0.0"), 0.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("-0.0"), 0.0);

  BOOST_CHECK_EQUAL(NumberParser::ToDouble("0.0e5"), 0.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("0.0e100"), 0.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("0.0e-100"), 0.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("0.0E5"), 0.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("0.0E100"), 0.0);
  BOOST_CHECK_EQUAL(NumberParser::ToDouble("0.0E-100"), 0.0);

  BOOST_CHECK_CLOSE(NumberParser::ToDouble("1.0e5"), 1.0e5, 1e-5);
  BOOST_CHECK_CLOSE(NumberParser::ToDouble("1.0e-5"), 1.0e-5, 1e-5);
  BOOST_CHECK_CLOSE(NumberParser::ToDouble("0.3e0"), 0.3, 1e-5);
  BOOST_CHECK_CLOSE(NumberParser::ToDouble("642.135e8"), 642.135e8, 1e-5);
}

BOOST_AUTO_TEST_SUITE_END()
