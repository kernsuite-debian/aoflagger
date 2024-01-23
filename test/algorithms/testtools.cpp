#include "testtools.h"

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"
#include "../../structures/timefrequencydata.h"

#include "../../algorithms/testsetgenerator.h"
#include "../../algorithms/thresholdconfig.h"
#include "../../algorithms/sumthreshold.h"
#include "../../algorithms/sumthresholdmissing.h"

#include <boost/test/unit_test.hpp>

#include <functional>
#include <set>

using aocommon::Polarization;

using algorithms::BackgroundTestSet;
using algorithms::RFITestSet;
using algorithms::SumThreshold;
using algorithms::TestSetGenerator;
using algorithms::ThresholdConfig;

namespace test_tools {

namespace {
constexpr size_t kFeatureSize = 16;
}

void CompareVerticalSumThreshold(SumThresholdFunction algorithm,
                                 const std::set<size_t>& skips) {
  const unsigned width = 512, height = 256;
  Mask2D mask = Mask2D::MakeUnsetMask(width, height);
  Mask2D scratch = Mask2D::MakeUnsetMask(width, height);
  Mask2D referenceMask = Mask2D::MakeUnsetMask(width, height);
  TimeFrequencyData data = TestSetGenerator::MakeTestSet(
      RFITestSet::GaussianBursts, BackgroundTestSet::Empty, width, height);
  Image2DCPtr image = data.GetSingleImage();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  num_t mode = image->GetMode();
  config.InitializeThresholdsFromFirstThreshold(6.0 * mode,
                                                ThresholdConfig::Rayleigh);
  for (unsigned i = 0; i < 9; ++i) {
    referenceMask.SetAll<false>();
    mask.SetAll<false>();

    const unsigned length = config.GetHorizontalLength(i);
    const double threshold = config.GetHorizontalThreshold(i);

    SumThreshold::VerticalLargeReference(image.get(), &referenceMask, &scratch,
                                         length, threshold);
    algorithm(image.get(), &mask, &scratch, length, threshold);

    if (skips.find(length) == skips.end()) {
      BOOST_CHECK_EQUAL(referenceMask.GetCount<true>(), mask.GetCount<true>());

      std::ostringstream str;
      str << "referenceMask == mask for length " << length;
      BOOST_CHECK_MESSAGE(referenceMask == mask, str.str());
    }
  }
}

void CompareHorizontalSumThreshold(SumThresholdFunction algorithm) {
  const size_t width = 512, height = 256;
  Mask2D referenceMask = Mask2D::MakeUnsetMask(width, height),
         mask = Mask2D::MakeUnsetMask(width, height),
         scratch = Mask2D::MakeUnsetMask(width, height);
  TimeFrequencyData data = TestSetGenerator::MakeTestSet(
      RFITestSet::GaussianBursts, BackgroundTestSet::Empty, width, height);
  Image2DPtr real(Image2D::MakePtr(*data.GetImage(0)));
  Image2DPtr imag(Image2D::MakePtr(*data.GetImage(1)));

  referenceMask.SwapXY();
  mask.SwapXY();
  real->Transpose();
  imag->Transpose();

  Image2DCPtr image =
      TimeFrequencyData(Polarization::XX, real, imag).GetSingleImage();

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  num_t mode = image->GetMode();
  config.InitializeThresholdsFromFirstThreshold(6.0 * mode,
                                                ThresholdConfig::Rayleigh);
  for (size_t i = 0; i < 9; ++i) {
    referenceMask.SetAll<false>();
    mask.SetAll<false>();

    const size_t length = config.GetHorizontalLength(i);
    const double threshold = config.GetHorizontalThreshold(i);

    SumThreshold::HorizontalLargeReference(image.get(), &referenceMask,
                                           &scratch, length, threshold);
    algorithm(image.get(), &mask, &scratch, length, threshold);

    std::ostringstream str;
    str << "referenceMask == mask for length " << length;
    BOOST_CHECK_MESSAGE(mask == referenceMask, str.str());
  }
}

void IntroduceGap(const Image2D& input, const Mask2D& mask, Image2D& mInput,
                  Mask2D& mMask, Mask2D& missing) {
  const size_t feature_size = (kFeatureSize > input.Width()) ? 1 : kFeatureSize;
  missing = Mask2D::MakeSetMask<false>(input.Width() + feature_size,
                                       input.Height() + feature_size);
  mMask = Mask2D::MakeUnsetMask(mask.Width() + feature_size,
                                mask.Height() + feature_size);
  mInput = Image2D::MakeUnsetImage(input.Width() + feature_size,
                                   input.Height() + feature_size);
  const size_t x1 = input.Width() / 2;
  const size_t x2 = x1 + feature_size;
  const size_t y1 = input.Height() / 2;
  const size_t y2 = y1 + feature_size;
  for (size_t y = 0; y != y1; ++y) {
    const num_t* dataRowIn = input.ValuePtr(0, y);
    num_t* dataRowOut = mInput.ValuePtr(0, y);
    std::copy_n(dataRowIn, x1, dataRowOut);
    std::copy_n(dataRowIn, feature_size, dataRowOut + x1);
    std::copy_n(dataRowIn + x1, input.Width() - x1, dataRowOut + x2);
    const bool* maskRowIn = mask.ValuePtr(0, y);
    bool* maskRowOut = mMask.ValuePtr(0, y);
    std::copy_n(maskRowIn, x1, maskRowOut);
    std::copy_n(maskRowIn, feature_size, maskRowOut + x1);
    std::copy_n(maskRowIn + x1, input.Width() - x1, maskRowOut + x2);
    std::fill_n(missing.ValuePtr(x1, y), feature_size, true);
  }
  for (size_t y = y1; y != y2; ++y) {
    const num_t* rowIn = input.ValuePtr(0, y);
    num_t* rowOut = mInput.ValuePtr(0, y);
    std::copy_n(rowIn, x1, rowOut);
    std::copy_n(rowIn, feature_size, rowOut + x1);
    std::copy_n(rowIn + x1, input.Width() - x1, rowOut + x2);
    std::fill_n(missing.ValuePtr(0, y), missing.Width(), true);
  }
  for (size_t y = y1; y != input.Height(); ++y) {
    const num_t* dataRowIn = input.ValuePtr(0, y);
    num_t* dataRowOut = mInput.ValuePtr(0, y + feature_size);
    std::copy_n(dataRowIn, x1, dataRowOut);
    std::copy_n(dataRowIn, feature_size, dataRowOut + x1);
    std::copy_n(dataRowIn + x1, input.Width() - x1, dataRowOut + x2);
    const bool* maskRowIn = mask.ValuePtr(0, y);
    bool* maskRowOut = mMask.ValuePtr(0, y + feature_size);
    std::copy_n(maskRowIn, x1, maskRowOut);
    std::copy_n(maskRowIn, feature_size, maskRowOut + x1);
    std::copy_n(maskRowIn + x1, input.Width() - x1, maskRowOut + x2);
    std::fill_n(missing.ValuePtr(x1, y), feature_size, true);
  }
}

void RemoveGap(Mask2D& mask, const Mask2D& mMask) {
  const size_t feature_size = (kFeatureSize > mask.Width()) ? 1 : kFeatureSize;
  const size_t x1 = mask.Width() / 2;
  const size_t x2 = std::min(x1 + feature_size, mask.Width());
  const size_t y1 = mask.Height() / 2;
  for (size_t y = 0; y != y1; ++y) {
    const bool* maskRowIn = mMask.ValuePtr(0, y);
    bool* maskRowOut = mask.ValuePtr(0, y);
    std::copy_n(maskRowIn, x1, maskRowOut);
    std::copy_n(maskRowIn + x2, mask.Width() - x1, maskRowOut + x1);
  }
  for (size_t y = y1; y != mask.Height(); ++y) {
    const bool* maskRowIn = mMask.ValuePtr(0, y + feature_size);
    bool* maskRowOut = mask.ValuePtr(0, y);
    std::copy_n(maskRowIn, x1, maskRowOut);
    std::copy_n(maskRowIn + x2, mask.Width() - x1, maskRowOut + x1);
  }
}

}  // namespace test_tools
