#include <boost/test/unit_test.hpp>

#include "../../interface/aoflagger.h"

#include "../../algorithms/testsetgenerator.h"

#include "../../structures/timefrequencydata.h"

#include <version.h>

using algorithms::TestSetGenerator;

BOOST_AUTO_TEST_SUITE(interface, *boost::unit_test::label("interface"))

BOOST_AUTO_TEST_CASE(find_strategy) {
  aoflagger::AOFlagger flagger;
  std::string defaultStrategy = flagger.FindStrategyFile();
  BOOST_CHECK_NE(defaultStrategy, "");
}

BOOST_AUTO_TEST_CASE(load_nonexisting_strategy) {
  aoflagger::AOFlagger flagger;
  std::string unknownStrategy(
      "/this is not a name for an existing file, or so I hope.notlua");
  BOOST_CHECK_THROW(flagger.LoadStrategyFile(unknownStrategy), std::exception);
}

BOOST_AUTO_TEST_CASE(make_image_set) {
  aoflagger::AOFlagger flagger;
  aoflagger::ImageSet imageSet = flagger.MakeImageSet(100u, 100u, 1u, 0.0f);
  BOOST_CHECK_NE(imageSet.ImageBuffer(0), nullptr);
  BOOST_CHECK_EQUAL(imageSet.Width(), 100);
  BOOST_CHECK_EQUAL(imageSet.Height(), 100);
  BOOST_CHECK_EQUAL(imageSet.ImageCount(), 1);
  BOOST_CHECK_GE(imageSet.HorizontalStride(), 100);
}

BOOST_AUTO_TEST_CASE(run_default_strategy_without_input) {
  aoflagger::AOFlagger flagger;
  aoflagger::Strategy strategy =
      flagger.LoadStrategyFile(flagger.FindStrategyFile());
  aoflagger::ImageSet imageSet = flagger.MakeImageSet(100u, 100u, 1u, 0.0f);
  aoflagger::FlagMask mask = strategy.Run(imageSet);
  BOOST_CHECK_EQUAL(mask.Buffer()[0], false);
}

BOOST_AUTO_TEST_CASE(run_default_strategy_with_input) {
  aoflagger::AOFlagger flagger;
  aoflagger::Strategy strategy =
      flagger.LoadStrategyFile(flagger.FindStrategyFile());
  // the default strategy ignores the input flags, so no use in setting flags to
  // true
  aoflagger::FlagMask inputMask = flagger.MakeFlagMask(100u, 100u, false);
  aoflagger::ImageSet imageSet = flagger.MakeImageSet(100u, 100u, 1u, 0.0f);
  aoflagger::FlagMask mask = strategy.Run(imageSet, inputMask);
  BOOST_CHECK_EQUAL(mask.Buffer()[0], false);
}

BOOST_AUTO_TEST_CASE(runs) {
  size_t width = 200, height = 50;
  Mask2D gtMask = Mask2D::MakeSetMask<false>(width, height);
  TimeFrequencyData data = TestSetGenerator::MakeTestSet(
      algorithms::RFITestSet::FullBandBursts,
      algorithms::BackgroundTestSet::Empty, width, height);
  Image2DCPtr real = data.GetImage(0);
  Image2DCPtr imag = data.GetImage(1);
  aoflagger::AOFlagger flagger;
  std::string strategyPath =
      flagger.FindStrategyFile(aoflagger::TelescopeId::GENERIC_TELESCOPE);
  BOOST_CHECK_NE(strategyPath, "");
  aoflagger::Strategy strategy = flagger.LoadStrategyFile(strategyPath);
  aoflagger::ImageSet imageSet = flagger.MakeImageSet(width, height, 2);
  aoflagger::FlagMask inputMask = flagger.MakeFlagMask(width, height);

  float* realBuffer = imageSet.ImageBuffer(0);
  float* imagBuffer = imageSet.ImageBuffer(1);
  for (size_t y = 0; y != height; ++y) {
    std::copy_n(real->ValuePtr(0, y), width,
                realBuffer + y * imageSet.HorizontalStride());
    std::copy_n(imag->ValuePtr(0, y), width,
                imagBuffer + y * imageSet.HorizontalStride());
    std::copy_n(gtMask.ValuePtr(0, y), width,
                inputMask.Buffer() + y * inputMask.HorizontalStride());
  }

  aoflagger::FlagMask flagMaskA = strategy.Run(imageSet);
  aoflagger::FlagMask flagMaskB = strategy.Run(imageSet, inputMask);

  Mask2D mask2DA = Mask2D::MakeUnsetMask(width, height);
  Mask2D mask2DB = Mask2D::MakeUnsetMask(width, height);
  for (size_t y = 0; y != height; ++y) {
    std::copy_n(flagMaskA.Buffer() + y * flagMaskA.HorizontalStride(), width,
                mask2DA.ValuePtr(0, y));
    std::copy_n(flagMaskB.Buffer() + y * flagMaskB.HorizontalStride(), width,
                mask2DB.ValuePtr(0, y));
  }
  BOOST_CHECK_GT(mask2DA.GetCount<true>(), 0);
  BOOST_CHECK_LT(mask2DA.GetCount<true>(), width * height / 5);
  BOOST_CHECK_GT(mask2DB.GetCount<true>(), 0);
  BOOST_CHECK_LT(mask2DB.GetCount<true>(), width * height / 5);
}

BOOST_AUTO_TEST_CASE(version) {
  short major, minor, subminor;
  aoflagger::AOFlagger::GetVersion(major, minor, subminor);
  BOOST_CHECK_EQUAL(major, AOFLAGGER_VERSION_MAJOR);
  BOOST_CHECK_EQUAL(minor, AOFLAGGER_VERSION_MINOR);
  BOOST_CHECK_EQUAL(subminor, AOFLAGGER_VERSION_SUBMINOR);
}

BOOST_AUTO_TEST_SUITE_END()
