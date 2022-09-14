#include "../../algorithms/thresholdconfig.h"
#include "../../algorithms/testsetgenerator.h"
#include "../../structures/timefrequencydata.h"

#include <boost/test/unit_test.hpp>

using algorithms::ThresholdConfig;

BOOST_AUTO_TEST_SUITE(threshold_config, *boost::unit_test::label("algorithms"))

BOOST_AUTO_TEST_CASE(image_with_nan) {
  const size_t width = 5;
  const size_t height = 20;
  Mask2D mask = Mask2D::MakeSetMask<false>(width, height);
  TimeFrequencyData data = algorithms::TestSetGenerator::MakeTestSet(
      algorithms::RFITestSet::Empty, algorithms::BackgroundTestSet::Empty,
      width, height);
  Image2D image(*data.GetSingleImage());
  const double mode = image.GetMode();
  image.SetValue(0, height / 2, std::numeric_limits<num_t>::quiet_NaN());

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  config.InitializeThresholdsFromFirstThreshold(100.0 * mode,
                                                ThresholdConfig::Rayleigh);
  config.Execute(&image, &mask, false, 1.0, 1.0);
  BOOST_CHECK_EQUAL(mask.GetCount<true>(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
