#include <boost/test/unit_test.hpp>

#include "../../interface/aoflagger.h"

BOOST_AUTO_TEST_SUITE(lua_scripting, *boost::unit_test::label("lua"))

BOOST_AUTO_TEST_CASE(raise_exception) {
  aoflagger::AOFlagger flagger;
  aoflagger::Strategy strategy = flagger.LoadStrategyString(
      "function execute(input)\n"
      "  error(\"test for raising an error\")\n"
      "end\n");
  aoflagger::ImageSet imageSet = flagger.MakeImageSet(10, 10, 1);
  BOOST_CHECK_THROW(strategy.Run(imageSet), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
