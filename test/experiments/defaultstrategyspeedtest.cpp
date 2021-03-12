#include "../../algorithms/combinatorialthresholder.h"
#include "../../algorithms/polarizationstatistics.h"
#include "../../algorithms/testsetgenerator.h"
#include "../../algorithms/thresholdconfig.h"
#include "../../algorithms/siroperator.h"
#include "../../algorithms/sumthreshold.h"

#include "../../lua/telescopefile.h"

#include "../../structures/timefrequencydata.h"

#include "../../util/logger.h"
#include "../../util/stopwatch.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(default_strategy_speed,
                      *boost::unit_test::label("experiment") *
                          boost::unit_test::disabled())

TimeFrequencyData prepareData() {
  const unsigned width = 50000, height = 256;
  Mask2D rfi = Mask2D::MakeUnsetMask(width, height);
  Image2DPtr xxReal = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, rfi, width, height)),
             xxImag = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, rfi, width, height)),
             xyReal = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, rfi, width, height)),
             xyImag = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, rfi, width, height)),
             yxReal = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, rfi, width, height)),
             yxImag = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, rfi, width, height)),
             yyReal = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, rfi, width, height)),
             yyImag = Image2D::MakePtr(
                 TestSetGenerator::MakeTestSet(26, rfi, width, height));
  TimeFrequencyData data = TimeFrequencyData::FromLinear(
      xxReal, xxImag, xyReal, xyImag, yxReal, yxImag, yyReal, yyImag);
  return data;
}

BOOST_AUTO_TEST_CASE(time_sumthreshold_n_hori) {
  TimeFrequencyData data = prepareData();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  num_t stddev = data.GetSingleImage()->GetStdDev();
  config.InitializeThresholdsFromFirstThreshold(6.0 * stddev,
                                                ThresholdConfig::Rayleigh);
  const size_t N = 100;
  double hor = 0.0, sseHor = 0.0, avxHorDumas = 0.0, selectedHor = 0.0;
  for (unsigned i = 0; i < 9; ++i) {
    const unsigned length = config.GetHorizontalLength(i);
    const double threshold = config.GetHorizontalThreshold(i);
    Image2DCPtr input = data.GetSingleImage();
    Mask2D scratch = Mask2D::MakeUnsetMask(input->Width(), input->Height());
    SumThreshold::VerticalScratch vScratch(input->Width(), input->Height());

    Mask2D mask(*data.GetSingleMask()), maskInp;
    Stopwatch watch(true);
    for (size_t j = 0; j != N; ++j) {
      maskInp = mask;
      SumThreshold::HorizontalLargeReference(input.get(), &maskInp, &scratch,
                                             length, threshold);
    }
    hor += watch.Seconds();
    Logger::Info << "Horizontal, length " << length << ": " << watch.ToString()
                 << '\n';

#ifdef __SSE__
    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != N; ++j) {
      maskInp = mask;
      SumThreshold::HorizontalLargeSSE(input.get(), &maskInp, &scratch, length,
                                       threshold);
    }
    sseHor += watch.Seconds();
    Logger::Info << "SSE Horizontal, length " << length << ": "
                 << watch.ToString() << '\n';
#endif

#ifdef __AVX2__
    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != N; ++j) {
      maskInp = mask;
      SumThreshold::HorizontalAVXDumas(input.get(), &maskInp, length,
                                       threshold);
    }
    avxHorDumas += watch.Seconds();
    Logger::Info << "AVX Horizontal Dumas, length " << length << ": "
                 << watch.ToString() << '\n';
#endif

    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != N; ++j) {
      maskInp = mask;
      SumThreshold::HorizontalLarge(input.get(), &maskInp, &scratch, length,
                                    threshold);
    }
    selectedHor += watch.Seconds();
    Logger::Info << "Selected horizontal, length " << length << ": "
                 << watch.ToString() << '\n';

    Logger::Info << "Summed values:\n"
                 << "- Horizontal ref  : " << hor << "\n"
                 << "- Horizontal SSE  : " << sseHor << "\n"
                 << "- Horizontal AVX D: " << avxHorDumas << "\n"
                 << "- Selected horiz  : " << selectedHor << "\n";
  }
}

BOOST_AUTO_TEST_CASE(time_sumthreshold_n_vert) {
  TimeFrequencyData data = prepareData();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  num_t stddev = data.GetSingleImage()->GetStdDev();
  config.InitializeThresholdsFromFirstThreshold(6.0 * stddev,
                                                ThresholdConfig::Rayleigh);
  const size_t N = 100;
  double vert = 0.0, sseVert = 0.0, avxVert = 0.0, avxVertDumas = 0.0,
         selectedVer = 0.0;
  for (unsigned i = 0; i < 9; ++i) {
    const unsigned length = config.GetHorizontalLength(i);
    const double threshold = config.GetHorizontalThreshold(i);
    Image2DCPtr input = data.GetSingleImage();
    Mask2D scratch = Mask2D::MakeUnsetMask(input->Width(), input->Height());
    SumThreshold::VerticalScratch vScratch(input->Width(), input->Height());

    Mask2D mask(*data.GetSingleMask()), maskInp;

    Stopwatch watch(true);
    for (size_t j = 0; j != N; ++j) {
      maskInp = mask;
      SumThreshold::VerticalLargeReference(input.get(), &maskInp, &scratch,
                                           length, threshold);
    }
    vert += watch.Seconds();
    Logger::Info << "Vertical, length " << length << ": " << watch.ToString()
                 << '\n';

#ifdef __SSE__
    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != N; ++j) {
      maskInp = mask;
      SumThreshold::VerticalLargeSSE(input.get(), &maskInp, &scratch, length,
                                     threshold);
    }
    sseVert += watch.Seconds();
    Logger::Info << "SSE Vertical, length " << length << ": "
                 << watch.ToString() << '\n';
#endif

#ifdef __AVX2__
    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != N; ++j) {
      maskInp = mask;
      SumThreshold::VerticalLargeAVX(input.get(), &maskInp, &scratch, length,
                                     threshold);
    }
    avxVert += watch.Seconds();
    Logger::Info << "AVX Vertical, length " << length << ": "
                 << watch.ToString() << '\n';

    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != N; ++j) {
      maskInp = mask;
      SumThreshold::VerticalAVXDumas(input.get(), &maskInp, &vScratch, length,
                                     threshold);
    }
    avxVertDumas += watch.Seconds();
    Logger::Info << "AVX Vertical Dumas, length " << length << ": "
                 << watch.ToString() << '\n';
#endif

    mask = *data.GetSingleMask();
    watch.Reset(true);
    for (size_t j = 0; j != N; ++j) {
      maskInp = mask;
      SumThreshold::VerticalLarge(input.get(), &maskInp, &scratch, &vScratch,
                                  length, threshold);
    }
    selectedVer += watch.Seconds();
    Logger::Info << "Selected vertical, length " << length << ": "
                 << watch.ToString() << '\n';

    Logger::Info << "Summed values:\n"
                 << "- Vertical ref    : " << vert << "\n"
                 << "- Vertical SSE    : " << sseVert << "\n"
                 << "- Vertical AVX    : " << avxVert << "\n"
                 << "- Vertical AVX D  : " << avxVertDumas << "\n"
                 << "- Selected vertic : " << selectedVer << "\n";
  }
}

BOOST_AUTO_TEST_SUITE_END()
