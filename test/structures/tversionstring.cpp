#include "../../structures/versionstring.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(version_string, *boost::unit_test::label("structures"))

BOOST_AUTO_TEST_CASE(construct_major) {
  VersionString v("3");
  BOOST_CHECK_EQUAL(v.Major(), 3);
  BOOST_CHECK_EQUAL(v.Minor(), 0);
  BOOST_CHECK_EQUAL(v.Subminor(), 0);
  BOOST_CHECK(!v.HasMinor());
  BOOST_CHECK(!v.HasSubminor());
  BOOST_CHECK_EQUAL(v.String(), "3");
}

BOOST_AUTO_TEST_CASE(construct_minor) {
  VersionString v("2.7");
  BOOST_CHECK_EQUAL(v.Major(), 2);
  BOOST_CHECK_EQUAL(v.Minor(), 7);
  BOOST_CHECK_EQUAL(v.Subminor(), 0);
  BOOST_CHECK(v.HasMinor());
  BOOST_CHECK(!v.HasSubminor());
  BOOST_CHECK_EQUAL(v.String(), "2.7");
}

BOOST_AUTO_TEST_CASE(construct_subminor) {
  VersionString v("3.14.15");
  BOOST_CHECK_EQUAL(v.Major(), 3);
  BOOST_CHECK_EQUAL(v.Minor(), 14);
  BOOST_CHECK_EQUAL(v.Subminor(), 15);
  BOOST_CHECK(v.HasMinor());
  BOOST_CHECK(v.HasSubminor());
  BOOST_CHECK_EQUAL(v.String(), "3.14.15");
}

BOOST_AUTO_TEST_CASE(construct_fail) {
  BOOST_CHECK_THROW(VersionString(""), std::exception);
  BOOST_CHECK_THROW(VersionString("fail"), std::exception);
  BOOST_CHECK_THROW(VersionString("3.4fail"), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()
