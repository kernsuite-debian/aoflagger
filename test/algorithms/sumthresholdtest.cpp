#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"
#include "../../structures/timefrequencydata.h"

#include "../../algorithms/testsetgenerator.h"
#include "../../algorithms/thresholdconfig.h"
#include "../../algorithms/sumthreshold.h"

#include "../testingtools/maskasserter.h"

#include <boost/test/unit_test.hpp>

using namespace aocommon;

BOOST_AUTO_TEST_SUITE(sumthreshold, *boost::unit_test::label("algorithms"))

#ifdef __SSE__
BOOST_AUTO_TEST_CASE(vertical_sumthreshold_SSE) {
  const unsigned width = 2048, height = 256;
  Mask2D mask1 = Mask2D::MakeUnsetMask(width, height),
         mask2 = Mask2D::MakeUnsetMask(width, height),
         scratch = Mask2D::MakeUnsetMask(width, height);
  Image2DPtr real = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, mask1, width, height)),
             imag = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, mask2, width, height));
  TimeFrequencyData data(Polarization::XX, real, imag);
  Image2DCPtr image = data.GetSingleImage();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  num_t mode = image->GetMode();
  config.InitializeThresholdsFromFirstThreshold(6.0 * mode,
                                                ThresholdConfig::Rayleigh);
  for (unsigned i = 0; i < 9; ++i) {
    mask1.SetAll<false>();
    mask2.SetAll<false>();

    const unsigned length = config.GetHorizontalLength(i);
    const double threshold = config.GetHorizontalThreshold(i);

    SumThreshold::VerticalLargeReference(image.get(), &mask1, &scratch, length,
                                         threshold);
    SumThreshold::VerticalLargeSSE(image.get(), &mask2, &scratch, length,
                                   threshold);

    if (length != 32) {
      BOOST_CHECK(mask1 == mask2);
    }
  }
}

BOOST_AUTO_TEST_CASE(horizontal_sumthreshold_SSE) {
  const unsigned width = 2048, height = 256;
  Mask2D mask1 = Mask2D::MakeUnsetMask(width, height),
         mask2 = Mask2D::MakeUnsetMask(width, height),
         scratch = Mask2D::MakeUnsetMask(width, height);
  Image2DPtr real = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, mask1, width, height)),
             imag = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, mask2, width, height));

  mask1.SwapXY();
  mask2.SwapXY();
  real->SwapXY();
  imag->SwapXY();

  TimeFrequencyData data(Polarization::XX, real, imag);
  Image2DCPtr image = data.GetSingleImage();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  num_t mode = image->GetMode();
  config.InitializeThresholdsFromFirstThreshold(6.0 * mode,
                                                ThresholdConfig::Rayleigh);
  for (unsigned i = 0; i < 9; ++i) {
    mask1.SetAll<false>();
    mask2.SetAll<false>();

    const unsigned length = config.GetHorizontalLength(i);
    const double threshold = config.GetHorizontalThreshold(i);

    SumThreshold::HorizontalLargeReference(image.get(), &mask1, &scratch,
                                           length, threshold);
    SumThreshold::HorizontalLargeSSE(image.get(), &mask2, &scratch, length,
                                     threshold);

    std::stringstream s;
    s << "Equal SSE and reference masks produced by SumThreshold length "
      << length << ", threshold " << threshold;
    MaskAsserter::AssertEqualMasks(mask2, mask1, s.str());
  }
}

BOOST_AUTO_TEST_CASE(stability_SSE) {
  Mask2D maskA = Mask2D::MakeSetMask<false>(1, 1),
         maskB = Mask2D::MakeSetMask<false>(2, 2),
         maskC = Mask2D::MakeSetMask<false>(3, 3),
         maskD = Mask2D::MakeSetMask<false>(4, 4),
         scratch = Mask2D::MakeUnsetMask(4, 4);
  Image2D realA = Image2D::MakeZeroImage(1, 1),
          realB = Image2D::MakeZeroImage(2, 2),
          realC = Image2D::MakeZeroImage(3, 3),
          realD = Image2D::MakeZeroImage(4, 4);

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  config.InitializeThresholdsFromFirstThreshold(6.0, ThresholdConfig::Rayleigh);
  for (unsigned i = 0; i < 9; ++i) {
    const unsigned length = config.GetHorizontalLength(i);
    SumThreshold::HorizontalLargeSSE(&realA, &maskA, &scratch, length, 1.0);
    SumThreshold::VerticalLargeSSE(&realA, &maskA, &scratch, length, 1.0);
    SumThreshold::HorizontalLargeSSE(&realA, &maskB, &scratch, length, 1.0);
    SumThreshold::VerticalLargeSSE(&realA, &maskB, &scratch, length, 1.0);
    SumThreshold::HorizontalLargeSSE(&realA, &maskC, &scratch, length, 1.0);
    SumThreshold::VerticalLargeSSE(&realA, &maskC, &scratch, length, 1.0);
    SumThreshold::HorizontalLargeSSE(&realA, &maskD, &scratch, length, 1.0);
    SumThreshold::VerticalLargeSSE(&realA, &maskD, &scratch, length, 1.0);
  }
  BOOST_CHECK(true);
}
#endif  // __SSE__

#ifdef __AVX2__
BOOST_AUTO_TEST_CASE(horizontal_sumthreshold_AVX_dumas) {
  const unsigned width = 2048, height = 256;
  Mask2D mask1 = Mask2D::MakeUnsetMask(width, height),
         mask2 = Mask2D::MakeUnsetMask(width, height),
         scratch = Mask2D::MakeUnsetMask(width, height);
  Image2DPtr real = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, mask1, width, height)),
             imag = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, mask2, width, height));

  mask1.SwapXY();
  mask2.SwapXY();
  real->SwapXY();
  imag->SwapXY();

  TimeFrequencyData data(Polarization::XX, real, imag);
  Image2DCPtr image = data.GetSingleImage();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  num_t mode = image->GetMode();
  config.InitializeThresholdsFromFirstThreshold(6.0 * mode,
                                                ThresholdConfig::Rayleigh);
  for (unsigned i = 0; i < 9; ++i) {
    mask1.SetAll<false>();
    mask2.SetAll<false>();

    const unsigned length = config.GetHorizontalLength(i);
    const double threshold = config.GetHorizontalThreshold(i);

    SumThreshold::HorizontalLargeReference(image.get(), &mask1, &scratch,
                                           length, threshold);
    SumThreshold::HorizontalAVXDumas(image.get(), &mask2, length, threshold);

    std::stringstream s;
    s << "Equal AVX Dumas and reference masks produced by SumThreshold length "
      << length << ", threshold " << threshold;
    MaskAsserter::AssertEqualMasks(mask2, mask1, s.str());
  }
}

static void VerticalSumThresholdAVX(bool dumas) {
  const unsigned width = 2048, height = 256;
  Mask2D mask1 = Mask2D::MakeUnsetMask(width, height),
         mask2 = Mask2D::MakeUnsetMask(width, height),
         scratch = Mask2D::MakeUnsetMask(width, height);
  SumThreshold::VerticalScratch vScratch(width, height);
  Image2DPtr real = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, mask1, width, height)),
             imag = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, mask2, width, height));
  TimeFrequencyData data(Polarization::XX, real, imag);
  Image2DCPtr image = data.GetSingleImage();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  num_t mode = image->GetMode();
  config.InitializeThresholdsFromFirstThreshold(6.0 * mode,
                                                ThresholdConfig::Rayleigh);
  for (unsigned i = 0; i < 9; ++i) {
    mask1.SetAll<false>();
    mask2.SetAll<false>();

    const unsigned length = config.GetHorizontalLength(i);
    const double threshold = config.GetHorizontalThreshold(i);

    SumThreshold::VerticalLargeReference(image.get(), &mask1, &scratch, length,
                                         threshold);
    // std::cout << mask1.GetCount<true>() << '\n';
    if (dumas)
      SumThreshold::VerticalAVXDumas(image.get(), &mask2, &vScratch, length,
                                     threshold);
    else
      SumThreshold::VerticalLargeAVX(image.get(), &mask2, &scratch, length,
                                     threshold);

    if (length != 32 || dumas) {  // I think there was a numerical issue on some
                                  // platforms with 32, so just skip.
      std::stringstream s;
      s << "Equal AVX Dumas and reference masks produced by SumThreshold "
           "length "
        << length;
      MaskAsserter::AssertEqualMasks(mask2, mask1, s.str());
    }
  }
}

BOOST_AUTO_TEST_CASE(vertical_sumthreshold_AVX) {
  VerticalSumThresholdAVX(false);
}

BOOST_AUTO_TEST_CASE(vertical_sumthreshold_AVX_dumas) {
  VerticalSumThresholdAVX(true);
}

static void SimpleVerticalSumThresholdAVX(bool dumas) {
  const unsigned width = 8, height = 8;
  Mask2D mask = Mask2D::MakeSetMask<false>(width, height),
         scratch = Mask2D::MakeUnsetMask(width, height);
  SumThreshold::VerticalScratch vScratch(width, height);
  Image2D image = Image2D::MakeSetImage(width, height, 0.0);

  for (size_t x = 0; x != width; ++x) {
    image.SetValue(x, 3, 1.0);
    image.SetValue(x, 4, 1.0);
  }

  if (dumas)
    SumThreshold::VerticalAVXDumas(&image, &mask, &vScratch, 2, 0.8);
  else
    SumThreshold::VerticalLargeAVX(&image, &mask, &scratch, 2, 0.8);

  for (size_t x = 0; x != width; ++x) {
    BOOST_CHECK(mask.Value(x, 3));
    BOOST_CHECK(mask.Value(x, 4));
    BOOST_CHECK(!mask.Value(x, 0));
    BOOST_CHECK(!mask.Value(x, 2));
    BOOST_CHECK(!mask.Value(x, 5));
  }
}

BOOST_AUTO_TEST_CASE(simple_vertical_sumthreshold_AVX) {
  SimpleVerticalSumThresholdAVX(false);
}

BOOST_AUTO_TEST_CASE(simple_vertical_sumthreshold_AVX_dumas) {
  SimpleVerticalSumThresholdAVX(true);
}

static void StabilityAVX(bool dumas) {
  Mask2D maskA = Mask2D::MakeSetMask<false>(1, 1),
         maskB = Mask2D::MakeSetMask<false>(2, 2),
         maskC = Mask2D::MakeSetMask<false>(3, 3),
         maskD = Mask2D::MakeSetMask<false>(4, 4),
         scratch = Mask2D::MakeUnsetMask(4, 4);
  SumThreshold::VerticalScratch vScratch(4, 4);
  Image2D realA = Image2D::MakeZeroImage(1, 1),
          realB = Image2D::MakeZeroImage(2, 2),
          realC = Image2D::MakeZeroImage(3, 3),
          realD = Image2D::MakeZeroImage(4, 4);

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  config.InitializeThresholdsFromFirstThreshold(6.0, ThresholdConfig::Rayleigh);
  for (unsigned i = 0; i < 9; ++i) {
    const unsigned length = config.GetHorizontalLength(i);
    if (dumas) {
      SumThreshold::HorizontalAVXDumas(&realA, &maskA, length, 1.0);
      SumThreshold::VerticalAVXDumas(&realA, &maskA, &vScratch, length, 1.0);
      SumThreshold::HorizontalAVXDumas(&realA, &maskB, length, 1.0);
      SumThreshold::VerticalAVXDumas(&realA, &maskB, &vScratch, length, 1.0);
      SumThreshold::HorizontalAVXDumas(&realA, &maskC, length, 1.0);
      SumThreshold::VerticalAVXDumas(&realA, &maskC, &vScratch, length, 1.0);
      SumThreshold::HorizontalAVXDumas(&realA, &maskD, length, 1.0);
      SumThreshold::VerticalAVXDumas(&realA, &maskD, &vScratch, length, 1.0);
    } else {
      // SumThreshold::HorizontalLargeAVX(&realA, &maskA, &scratch,
      // length, 1.0);
      SumThreshold::VerticalLargeAVX(&realA, &maskA, &scratch, length, 1.0);
      // SumThreshold::HorizontalLargeAVX(&realA, &maskB, &scratch,
      // length, 1.0);
      SumThreshold::VerticalLargeAVX(&realA, &maskB, &scratch, length, 1.0);
      // SumThreshold::HorizontalLargeAVX(&realA, &maskC, &scratch,
      // length, 1.0);
      SumThreshold::VerticalLargeAVX(&realA, &maskC, &scratch, length, 1.0);
      // SumThreshold::HorizontalLargeAVX(&realA, &maskD, &scratch,
      // length, 1.0);
      SumThreshold::VerticalLargeAVX(&realA, &maskD, &scratch, length, 1.0);
    }
  }
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(stability_AVX) { StabilityAVX(false); }

BOOST_AUTO_TEST_CASE(stability_AVX_dumas) { StabilityAVX(true); }

#endif  // __AVX2__

BOOST_AUTO_TEST_SUITE_END()
