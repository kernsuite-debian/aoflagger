#include <boost/test/unit_test.hpp>

#include "../../interface/aoflagger.h"

BOOST_AUTO_TEST_SUITE(lua_metadata, *boost::unit_test::label("lua"))

BOOST_AUTO_TEST_CASE(no_metadata) {
  aoflagger::AOFlagger flagger;
  aoflagger::Strategy strategy = flagger.LoadStrategyString(
      "function execute(input)\n"
      "  assert(not input:has_metadata())\n"
      "  if(input:is_auto_correlation()) then\n"
      "    error(\"incorrect input:is_auto_correlation()\")\n"
      "  end\n"
      "end\n");
  aoflagger::ImageSet imageSet = flagger.MakeImageSet(10, 10, 1);
  strategy.Run(imageSet);
}

BOOST_AUTO_TEST_CASE(propagation) {
  aoflagger::AOFlagger flagger;
  aoflagger::Strategy strategy = flagger.LoadStrategyString(
      // Of course this could be done a lot shorter with Lua's assert, but
      // assert does not provide direct information about what went wrong
      "function execute(input)\n"
      "  if(not input:has_metadata()) then\n"
      "    error(\"input:has_metadata() should return true\")\n"
      "  end\n"
      "  if(input:get_antenna1_name() ~= \"first\") then\n"
      "    error(\"incorrect input:get_antenna1_name()\")\n"
      "  end\n"
      "  if(input:get_antenna2_name() ~= \"second\") then\n"
      "    error(\"incorrect input:get_antenna2_name()\")\n"
      "  end\n"
      "  if(input:get_antenna1_index() ~= 0) then\n"
      "    error(\"incorrect input:get_antenna1_index()\")\n"
      "  end\n"
      "  if(input:get_antenna2_index() ~= 1) then\n"
      "    error(\"incorrect input:get_antenna2_index()\")\n"
      "  end\n"
      "  if(input:get_baseline_distance() ~= 5) then\n"
      "    error(\"incorrect input:get_baseline_distance()\")\n"
      "  end\n"
      "  local v = input:get_baseline_vector()\n"
      "  if(v['x'] ~= 3) then\n"
      "    error(\"incorrect input:get_baseline_vector()['x']\")\n"
      "  end\n"
      "  if(v['y'] ~= 0) then\n"
      "    error(\"incorrect input:get_baseline_vector()['y']\")\n"
      "  end\n"
      "  if(v['z'] ~= 4) then\n"
      "    error(\"incorrect input:get_baseline_vector()['z']\")\n"
      "  end\n"
      "  if(#input:get_frequencies() ~= 10) then\n"
      "    error(\"incorrect #input:get_frequencies()\")\n"
      "  end\n"
      "  if(input:get_frequencies()[1] ~= 150e6) then\n"
      "    error(\"incorrect "
      "input:get_frequencies()[1]\"..input:get_frequencies()[1])\n"
      "  end\n"
      "  if(#input:get_times() ~= 8) then\n"
      "    error(\"incorrect #input:get_times()\")\n"
      "  end\n"
      "  if(input:is_auto_correlation()) then\n"
      "    error(\"incorrect input:is_auto_correlation()\")\n"
      "  end\n"
      "end\n");
  aoflagger::Antenna a1, a2;
  a1.id = 0;
  a1.name = "first";
  a1.x = 1;
  a1.y = 0;
  a1.z = 2;
  a2.id = 1;
  a2.name = "second";
  a2.x = 4;
  a2.y = 0;
  a2.z = 6;
  flagger.SetAntennaList(std::vector<aoflagger::Antenna>{a1, a2});

  aoflagger::Channel c;
  c.width = 1e6;
  c.frequency = 150e6;
  aoflagger::Band b;
  b.id = 0;
  b.channels.assign(10, c);
  flagger.SetBandList(std::vector<aoflagger::Band>{b});

  aoflagger::Interval interval;
  interval.id = 0;
  interval.times.assign(8, 1337.0);
  flagger.SetIntervalList(std::vector<aoflagger::Interval>{interval});

  aoflagger::ImageSet imageSet = flagger.MakeImageSet(8, 10, 1);
  imageSet.SetAntennas(0, 1);
  imageSet.SetBand(0);
  imageSet.SetInterval(0);
  strategy.Run(imageSet);
}

BOOST_AUTO_TEST_SUITE_END()
