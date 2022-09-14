
#include <boost/test/unit_test.hpp>

#include "../../lua/default-strategy.h"
#include "../../lua/luastrategy.h"
#include "../../lua/scriptdata.h"

#include "../../algorithms/testsetgenerator.h"

using aocommon::Polarization;
using aocommon::PolarizationEnum;

using algorithms::BackgroundTestSet;
using algorithms::RFITestSet;
using algorithms::TestSetGenerator;

BOOST_AUTO_TEST_SUITE(default_strategy, *boost::unit_test::label("lua"))

static void initStrategy(LuaStrategy& strategy) {
  strategy.Initialize();
  std::vector<char> str(data_strategies_generic_default_lua_len + 1, 0);
  std::copy_n(data_strategies_generic_default_lua,
              data_strategies_generic_default_lua_len, str.data());
  strategy.LoadText(std::string(str.data()));
}

static std::pair<double, double> checkResult(TimeFrequencyData& data,
                                             const Mask2D& groundTruth) {
  LuaStrategy strategy;
  initStrategy(strategy);
  ScriptData scriptData;
  TimeFrequencyMetaDataCPtr metaData(new TimeFrequencyMetaData());
  TimeFrequencyData result = data;
  result.SetNoMask();
  strategy.Execute(result, metaData, scriptData, "execute");

  BOOST_REQUIRE_GT(result.MaskCount(), 0);
  size_t width = groundTruth.Width(), height = groundTruth.Height();
  size_t posCount = result.GetSingleMask()->GetCount<true>();
  Mask2D intersect(groundTruth);
  intersect.Intersect(*result.GetSingleMask());

  size_t truePosCount = intersect.GetCount<true>();
  size_t falsePosCount = posCount - truePosCount;
  size_t falseNegCount =
      groundTruth.GetCount<true>() - intersect.GetCount<true>();
  double fpRatio = double(falsePosCount) / double(width * height);
  double fnRatio = double(falseNegCount) / double(width * height);
  return std::make_pair(fpRatio, fnRatio);
}

BOOST_AUTO_TEST_CASE(stokesi_complex) {
  size_t width = 200, height = 50;
  TimeFrequencyData data = TestSetGenerator::MakeTestSet(
      RFITestSet::FullBandBursts, BackgroundTestSet::Empty, width, height);

  auto fpfn = checkResult(data, *data.GetSingleMask());
  BOOST_CHECK_LT(fpfn.first, 5e-3);  // False positives-ratio should be < 0.5 %
  BOOST_CHECK_LT(fpfn.second,
                 0.015);  // False negatives-ratio should be < 1.5 %
}

BOOST_AUTO_TEST_CASE(stokesi_amplitude) {
  size_t width = 200, height = 50;
  Mask2D mask = Mask2D::MakeSetMask<false>(width, height);
  TimeFrequencyData data = TestSetGenerator::MakeTestSet(
      RFITestSet::FullBandBursts, BackgroundTestSet::Empty, width, height);
  Image2DCPtr amplitude = data.GetImage(0);

  TimeFrequencyData tfData(TimeFrequencyData::AmplitudePart,
                           Polarization::StokesI, amplitude);

  auto fpfn = checkResult(tfData, *data.GetSingleMask());
  BOOST_CHECK_LT(fpfn.first, 5e-3);   // False positives-ratio should be < 0.5 %
  BOOST_CHECK_LT(fpfn.second, 5e-3);  // False negatives-ratio should be < 0.5 %
}

BOOST_AUTO_TEST_CASE(full_polarization) {
  size_t width = 200, height = 50;
  TimeFrequencyData allData;
  PolarizationEnum pols[4] = {Polarization::XX, Polarization::XY,
                              Polarization::YX, Polarization::YY};
  for (size_t i = 0; i != 4; ++i) {
    TimeFrequencyData real = TestSetGenerator::MakeTestSet(
        RFITestSet::FullBandBursts, BackgroundTestSet::Empty, width, height);
    TimeFrequencyData imag = TestSetGenerator::MakeTestSet(
        RFITestSet::FullBandBursts, BackgroundTestSet::Empty, width, height);
    TimeFrequencyData tfData(pols[i], real.GetImage(0), imag.GetImage(0));
    tfData.SetGlobalMask(real.GetMask(0));
    if (i == 0)
      allData = tfData;
    else
      allData =
          TimeFrequencyData::MakeFromPolarizationCombination(allData, tfData);
  }

  auto fpfn = checkResult(allData, *allData.GetSingleMask());
  BOOST_CHECK_LT(fpfn.first, 2e-2);   // False positives-ratio should be < 2 %
  BOOST_CHECK_LT(fpfn.second, 2e-3);  // False negatives-ratio should be < 0.2 %
}

BOOST_AUTO_TEST_SUITE_END()
