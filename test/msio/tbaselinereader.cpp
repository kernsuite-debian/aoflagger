#include "msio/baselinereader.h"

#include "test/config.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(baseline_reader)

BOOST_AUTO_TEST_CASE(test_measurement_set_interval_data_size) {
  const std::string ms_name{kTestMsSize};

  // Validate the expected size.
  BOOST_REQUIRE(BaselineReader::MeasurementSetDataSize(ms_name) == 39'528'000);

  // Empty range selected.
  BOOST_REQUIRE(BaselineReader::MeasurementSetIntervalDataSize(ms_name, 0, 0) ==
                0);

  // Full range selected.
  BOOST_REQUIRE(BaselineReader::MeasurementSetIntervalDataSize(
                    ms_name, 0, 150) == 39'528'000);

  // No interval set -> full range.
  BOOST_REQUIRE(BaselineReader::MeasurementSetIntervalDataSize(
                    ms_name, std::optional<size_t>{},
                    std::optional<size_t>{}) == 39'528'000);

  // 10% sliding
  for (size_t i = 0; i < 10; ++i)
    BOOST_REQUIRE(BaselineReader::MeasurementSetIntervalDataSize(
                      ms_name, 10 * i, 10 * i + 15) == 3'952'800);

  // 20% -> 100% 10% step size (10% is already done before.)
  for (size_t i = 2; i <= 10; ++i)
    BOOST_REQUIRE(BaselineReader::MeasurementSetIntervalDataSize(
                      ms_name, 0, 15 * i) == 3'952'800 * i);
}

BOOST_AUTO_TEST_SUITE_END()
