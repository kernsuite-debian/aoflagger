#include "../../algorithms/combinatorialthresholder.h"
#include "../../algorithms/polarizationstatistics.h"
#include "../../algorithms/testsetgenerator.h"
#include "../../algorithms/thresholdconfig.h"
#include "../../algorithms/siroperator.h"
#include "../../algorithms/sumthreshold.h"
#include "../../algorithms/sumthresholdmissing.h"

#include "../../lua/telescopefile.h"

#include "../../structures/timefrequencydata.h"

#include "../../util/logger.h"
#include "../../util/stopwatch.h"

#include <boost/test/unit_test.hpp>

using algorithms::BackgroundTestSet;
using algorithms::RFITestSet;
using algorithms::SumThreshold;
using algorithms::SumThresholdMissing;
using algorithms::TestSetGenerator;
using algorithms::ThresholdConfig;

namespace {
constexpr size_t nRepeats = 3;
constexpr size_t width = 50000, height = 256;
}  // namespace

BOOST_AUTO_TEST_SUITE(default_strategy_speed,
                      *boost::unit_test::label("experiment") *
                          boost::unit_test::disabled())

TimeFrequencyData prepareData() {
  TimeFrequencyData data = TestSetGenerator::MakeTestSet(
      RFITestSet::GaussianBursts, BackgroundTestSet::Empty, width, height);
  return data.Make(TimeFrequencyData::AmplitudePart);
}

BOOST_AUTO_TEST_CASE(time_masked_sumthreshold_full) {
  TimeFrequencyData data = prepareData();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  const num_t stddev = data.GetSingleImage()->GetStdDev();
  config.InitializeThresholdsFromFirstThreshold(6.0 * stddev,
                                                ThresholdConfig::Rayleigh);

  Image2DCPtr image = data.GetSingleImage();
  Mask2D zero = Mask2D::MakeSetMask<false>(image->Width(), image->Height());
  Mask2D mask(zero);

  double t = 0.0;
  for (size_t i = 0; i != 10; ++i) {
    Stopwatch watch(true);
    config.ExecuteWithMissing(image.get(), &mask, &zero, true, 1.0, 1.0);
    t += watch.Seconds();
    mask = zero;
  }
  std::cout << "10 runs: " << t << '\n';
}

BOOST_AUTO_TEST_CASE(time_sumthreshold_n_hori) {
  TimeFrequencyData data = prepareData();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  const num_t stddev = data.GetSingleImage()->GetStdDev();
  config.InitializeThresholdsFromFirstThreshold(6.0 * stddev,
                                                ThresholdConfig::Rayleigh);
  double hor = 0.0, sseHor = 0.0, avxHorDumas = 0.0, selectedHor = 0.0,
         missing = 0.0;
  for (unsigned i = 0; i < 9; ++i) {
    const unsigned length = config.GetHorizontalLength(i);
    const double threshold = config.GetHorizontalThreshold(i);
    Image2DCPtr input = data.GetSingleImage();
    Mask2D scratch = Mask2D::MakeUnsetMask(input->Width(), input->Height());
    SumThreshold::VerticalScratch vScratch(input->Width(), input->Height());

    Mask2D mask(*data.GetSingleMask()), maskInp;
    Mask2D zero = Mask2D::MakeSetMask<false>(mask.Width(), mask.Height());
    Stopwatch watch(true);
    for (size_t j = 0; j != nRepeats; ++j) {
      maskInp = mask;
      SumThreshold::HorizontalLargeReference(input.get(), &maskInp, &scratch,
                                             length, threshold);
    }
    hor += watch.Seconds();
    Logger::Info << "Horizontal, length " << length << ": " << watch.ToString()
                 << '\n';

    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != nRepeats; ++j) {
      maskInp = mask;
      SumThresholdMissing::Horizontal(*input, maskInp, zero, scratch, length,
                                      threshold);
    }
    missing += watch.Seconds();
    Logger::Info << "Horizontal with missing, length " << length << ": "
                 << watch.ToString() << '\n';

#if defined(__SSE__) || defined(__x86_64__)
    if (__builtin_cpu_supports("sse")) {
      mask = *data.GetSingleMask();
      watch.Reset(true);
      for (size_t j = 0; j != nRepeats; ++j) {
        maskInp = mask;
        SumThreshold::HorizontalLargeSSE(input.get(), &maskInp, &scratch,
                                         length, threshold);
      }
      sseHor += watch.Seconds();
      Logger::Info << "SSE Horizontal, length " << length << ": "
                   << watch.ToString() << '\n';
    }
#endif

#if defined(__AVX2__) || defined(__x86_64__)
    if (__builtin_cpu_supports("avx2")) {
      mask = *data.GetSingleMask();
      watch.Reset(true);
      for (size_t j = 0; j != nRepeats; ++j) {
        maskInp = mask;
        SumThreshold::HorizontalAVXDumas(input.get(), &maskInp, length,
                                         threshold);
      }
      avxHorDumas += watch.Seconds();
      Logger::Info << "AVX Horizontal Dumas, length " << length << ": "
                   << watch.ToString() << '\n';
    }
#endif

    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != nRepeats; ++j) {
      maskInp = mask;
      SumThreshold::HorizontalLarge(input.get(), &maskInp, &scratch, length,
                                    threshold);
    }
    selectedHor += watch.Seconds();
    Logger::Info << "Selected horizontal, length " << length << ": "
                 << watch.ToString() << '\n';

    Logger::Info << "Summed values:\n"
                 << "- Horizontal ref  : " << hor << "\n"
                 << "- Horizontal missing  : " << missing << "\n"
                 << "- Horizontal SSE  : " << sseHor << "\n"
                 << "- Horizontal AVX D: " << avxHorDumas << "\n"
                 << "- Selected horiz  : " << selectedHor << "\n";
  }
}

BOOST_AUTO_TEST_CASE(time_sumthreshold_n_vert) {
  TimeFrequencyData data = prepareData();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  const num_t stddev = data.GetSingleImage()->GetStdDev();
  config.InitializeThresholdsFromFirstThreshold(6.0 * stddev,
                                                ThresholdConfig::Rayleigh);
  double vert = 0.0, sseVert = 0.0, avxVert = 0.0, avxVertDumas = 0.0,
         missingRef = 0.0, missingStacked = 0.0, missingConsecutive = 0.0,
         selectedVer = 0.0;
  for (unsigned i = 0; i < 9; ++i) {
    const unsigned length = config.GetHorizontalLength(i);
    const double threshold = config.GetHorizontalThreshold(i);
    Image2DCPtr input = data.GetSingleImage();
    Mask2D scratch = Mask2D::MakeUnsetMask(input->Width(), input->Height());
    SumThreshold::VerticalScratch vScratch(input->Width(), input->Height());

    Mask2D mask(*data.GetSingleMask()), maskInp;
    Mask2D zero = Mask2D::MakeSetMask<false>(mask.Width(), mask.Height());

    Stopwatch watch(true);
    for (size_t j = 0; j != nRepeats; ++j) {
      maskInp = mask;
      SumThreshold::VerticalLargeReference(input.get(), &maskInp, &scratch,
                                           length, threshold);
    }
    vert += watch.Seconds();
    Logger::Info << "Vertical, length " << length << ": " << watch.ToString()
                 << '\n';

    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != nRepeats; ++j) {
      maskInp = mask;
      SumThresholdMissing::VerticalReference(*input, maskInp, zero, scratch,
                                             length, threshold);
    }
    missingRef += watch.Seconds();
    Logger::Info << "Vertical missing ref, length " << length << ": "
                 << watch.ToString() << '\n';

    mask = *data.GetSingleMask();
    watch.Reset(true);
    SumThresholdMissing::VerticalCache vCache;
    SumThresholdMissing::InitializeVertical(vCache, *input, zero);
    for (size_t j = 0; j != nRepeats; ++j) {
      maskInp = mask;
      SumThresholdMissing::VerticalStacked(vCache, *input, maskInp, zero,
                                           scratch, length, threshold);
    }
    missingStacked += watch.Seconds();
    Logger::Info << "Vertical missing stacked, length " << length << ": "
                 << watch.ToString() << '\n';

    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != nRepeats; ++j) {
      maskInp = mask;
      SumThresholdMissing::VerticalConsecutive(*input, maskInp, zero, scratch,
                                               length, threshold);
    }
    missingConsecutive += watch.Seconds();
    Logger::Info << "Vertical missing consecutive, length " << length << ": "
                 << watch.ToString() << '\n';

#if defined(__SSE__) || defined(__x86_64__)
    if (__builtin_cpu_supports("sse")) {
      mask = *data.GetSingleMask();
      watch.Reset(true);
      for (size_t j = 0; j != nRepeats; ++j) {
        maskInp = mask;
        SumThreshold::VerticalLargeSSE(input.get(), &maskInp, &scratch, length,
                                       threshold);
      }
      sseVert += watch.Seconds();
      Logger::Info << "SSE Vertical, length " << length << ": "
                   << watch.ToString() << '\n';
    }
#endif

#if defined(__AVX2__) || defined(__x86_64__)
    if (__builtin_cpu_supports("avx2")) {
      mask = *data.GetSingleMask();
      watch.Reset(true);
      for (size_t j = 0; j != nRepeats; ++j) {
        maskInp = mask;
        SumThreshold::VerticalLargeAVX(input.get(), &maskInp, &scratch, length,
                                       threshold);
      }
      avxVert += watch.Seconds();
      Logger::Info << "AVX Vertical, length " << length << ": "
                   << watch.ToString() << '\n';

      mask = *data.GetSingleMask();
      watch.Reset(true);
      for (size_t j = 0; j != nRepeats; ++j) {
        maskInp = mask;
        SumThreshold::VerticalAVXDumas(input.get(), &maskInp, &vScratch, length,
                                       threshold);
      }
      avxVertDumas += watch.Seconds();
      Logger::Info << "AVX Vertical Dumas, length " << length << ": "
                   << watch.ToString() << '\n';
    }
#endif

    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != nRepeats; ++j) {
      maskInp = mask;
      SumThreshold::VerticalLarge(input.get(), &maskInp, &scratch, &vScratch,
                                  length, threshold);
    }
    selectedVer += watch.Seconds();
    Logger::Info << "Selected vertical, length " << length << ": "
                 << watch.ToString() << '\n';

    Logger::Info << "Summed values:\n"
                 << "- Vertical ref        : " << vert << "\n"
                 << "- Vertical missing ref: " << missingRef << "\n"
                 << "- Vertical m. stacked : " << missingStacked << "\n"
                 << "- Vertical m. consec  : " << missingConsecutive << "\n"
                 << "- Vertical SSE        : " << sseVert << "\n"
                 << "- Vertical AVX        : " << avxVert << "\n"
                 << "- Vertical AVX D      : " << avxVertDumas << "\n"
                 << "- Selected vertic     : " << selectedVer << "\n";
  }
}

BOOST_AUTO_TEST_SUITE_END()
