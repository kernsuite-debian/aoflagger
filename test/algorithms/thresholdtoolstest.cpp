#include "../../structures/mask2d.h"

#include "../../algorithms/thresholdtools.h"

#include <boost/test/unit_test.hpp>

using algorithms::ThresholdTools;

BOOST_AUTO_TEST_SUITE(threshold_tools, *boost::unit_test::label("algorithms"))

BOOST_AUTO_TEST_CASE(winsorized_masked_mean_var) {
  num_t mean, variance;

  Mask2DCPtr fullMask = Mask2D::CreateSetMaskPtr<true>(100, 1);
  Image2DPtr image1 = Image2D::CreateZeroImagePtr(100, 1);
  // we just don't want it to crash
  ThresholdTools::WinsorizedMeanAndStdDev(image1.get(), fullMask.get(), mean,
                                          variance);

  Mask2DCPtr emptyMask = Mask2D::CreateSetMaskPtr<false>(100, 1);

  ThresholdTools::WinsorizedMeanAndStdDev(image1.get(), emptyMask.get(), mean,
                                          variance);
  BOOST_CHECK_CLOSE(mean, 0.0, 1e-5);
  BOOST_CHECK_CLOSE(variance, 0.0, 1e-5);

  image1->SetValue(0, 0, 1.0);
  ThresholdTools::WinsorizedMeanAndStdDev(image1.get(), emptyMask.get(), mean,
                                          variance);
  BOOST_CHECK_CLOSE(mean, 0.0, 1e-5);
  BOOST_CHECK_CLOSE(variance, 0.0, 1e-5);

  image1->SetValue(1, 0, -2.0);
  ThresholdTools::WinsorizedMeanAndStdDev(image1.get(), emptyMask.get(), mean,
                                          variance);
  BOOST_CHECK_CLOSE(mean, 0.0, 1e-5);
  BOOST_CHECK_CLOSE(variance, 0.0, 1e-5);

  Mask2DPtr mask1 = Mask2D::CreateSetMaskPtr<false>(100, 1);
  for (unsigned x = 50; x < 100; ++x) mask1->SetValue(x, 0, true);
  ThresholdTools::WinsorizedMeanAndStdDev(image1.get(), mask1.get(), mean,
                                          variance);
  BOOST_CHECK_CLOSE(mean, 0.0, 1e-5);
  BOOST_CHECK_CLOSE(variance, 0.0, 1e-5);

  for (unsigned x = 0; x < 50; ++x) image1->SetValue(x, 0, 1.0);
  ThresholdTools::WinsorizedMeanAndStdDev(image1.get(), mask1.get(), mean,
                                          variance);
  BOOST_CHECK_CLOSE(mean, 1.0, 1e-5);
  BOOST_CHECK_CLOSE(variance, 0.0, 1e-5);

  for (unsigned x = 0; x < 25; ++x) image1->SetValue(x, 0, -1.0);
  ThresholdTools::WinsorizedMeanAndStdDev(image1.get(), mask1.get(), mean,
                                          variance);
  BOOST_CHECK_CLOSE(mean, 0.0, 1e-5);
  // Remember: since the distribution is not Gaussian, the variance does not
  // correspond with the Winsorized variance. The sqrtn(1.54) should be the
  // correction term.
  BOOST_CHECK_CLOSE(variance, 1.0 * sqrtn(1.54), 1e-5);

  for (unsigned x = 0; x < 100; ++x) image1->SetValue(x, 0, x + 1.0);
  ThresholdTools::WinsorizedMeanAndStdDev(image1.get(), mask1.get(), mean,
                                          variance);
  BOOST_CHECK_LT(fabs(mean - 25.5), 0.2);
  // Since the distribution is not Gaussian, the variance does not correspond
  // with the Winsorized variance. Therefore, don't test it here. TODO
}

BOOST_AUTO_TEST_CASE(winsorized_masked_mode) {
  num_t mode;
  Mask2DCPtr fullMask = Mask2D::CreateSetMaskPtr<true>(100, 1);
  Image2DPtr image1 = Image2D::CreateZeroImagePtr(100, 1);
  // we just don't want it to crash
  ThresholdTools::WinsorizedMode(image1.get(), fullMask.get());

  Mask2DCPtr emptyMask = Mask2D::CreateSetMaskPtr<false>(100, 1);
  mode = ThresholdTools::WinsorizedMode(image1.get(), emptyMask.get());
  BOOST_CHECK_CLOSE(mode, 0.0, 1e-5);

  image1->SetValue(0, 0, 1.0);
  mode = ThresholdTools::WinsorizedMode(image1.get(), emptyMask.get());
  BOOST_CHECK_CLOSE(mode, 0.0, 1e-5);

  Mask2DPtr mask1 = Mask2D::CreateSetMaskPtr<false>(100, 1);
  for (unsigned x = 50; x < 100; ++x) mask1->SetValue(x, 0, true);
  mode = ThresholdTools::WinsorizedMode(image1.get(), mask1.get());
  BOOST_CHECK_CLOSE(mode, 0.0, 1e-5);

  for (unsigned x = 0; x < 50; ++x) image1->SetValue(x, 0, 1.0);
  mode = ThresholdTools::WinsorizedMode(image1.get(), mask1.get());
  BOOST_CHECK_CLOSE(mode, sqrt(0.5) * 1.0541, 1e-5);

  // for(unsigned x=0;x<100;++x)
  //	image1->SetValue(x, 0, x+1.0);
  // mode = ThresholdTools::WinsorizedMeanAndStdDev(image1, mask1, mean,
  // variance); BOOST_CHECK_CLOSE(mean, 25.5, "Mean of 50% flagged sequencial
  // image");
  // Since the distribution is not Gaussian, the variance does not correspond
  // with the Winsorized variance. Therefore, don't test it here. TODO
}

BOOST_AUTO_TEST_SUITE_END()
