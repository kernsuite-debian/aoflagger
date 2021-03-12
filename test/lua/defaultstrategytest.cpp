
#include <boost/test/unit_test.hpp>

#include "../../lua/default-strategy.h"
#include "../../lua/luastrategy.h"
#include "../../lua/scriptdata.h"

#include "../../algorithms/testsetgenerator.h"

using namespace aocommon;

BOOST_AUTO_TEST_SUITE(default_strategy, *boost::unit_test::label("lua"))

static void initStrategy(LuaStrategy& strategy) {
  strategy.Initialize();
  std::vector<char> str(data_strategies_generic_default_lua_len + 1, 0);
  std::copy_n(data_strategies_generic_default_lua,
              data_strategies_generic_default_lua_len, str.data());
  strategy.LoadText(std::string(str.data()));
}

static std::pair<double, double> checkResult(TimeFrequencyData& data,
                                             Mask2D& groundTruth) {
  LuaStrategy strategy;
  initStrategy(strategy);
  ScriptData scriptData;
  TimeFrequencyMetaDataCPtr metaData(new TimeFrequencyMetaData());
  strategy.Execute(data, metaData, scriptData, "execute");

  BOOST_REQUIRE_GT(data.MaskCount(), 0);
  size_t width = groundTruth.Width(), height = groundTruth.Height();
  size_t posCount = data.GetSingleMask()->GetCount<true>();
  Mask2D intersect(groundTruth);
  intersect.Intersect(*data.GetSingleMask());

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
  Mask2D mask = Mask2D::MakeSetMask<false>(width, height);
  Image2D real = TestSetGenerator::MakeTestSet(3, mask, width, height),
          imag = TestSetGenerator::MakeTestSet(3, mask, width, height);

  TimeFrequencyData tfData(Polarization::StokesI, Image2D::MakePtr(real),
                           Image2D::MakePtr(imag));

  auto fpfn = checkResult(tfData, mask);
  BOOST_CHECK_LT(fpfn.first, 5e-3);   // False positives-ratio should be < 0.5 %
  BOOST_CHECK_LT(fpfn.second, 5e-3);  // False negatives-ratio should be < 0.5 %
}

BOOST_AUTO_TEST_CASE(stokesi_amplitude) {
  size_t width = 200, height = 50;
  Mask2D mask = Mask2D::MakeSetMask<false>(width, height);
  Image2D amplitude = TestSetGenerator::MakeTestSet(3, mask, width, height);

  TimeFrequencyData tfData(TimeFrequencyData::AmplitudePart,
                           Polarization::StokesI, Image2D::MakePtr(amplitude));

  auto fpfn = checkResult(tfData, mask);
  BOOST_CHECK_LT(fpfn.first, 5e-3);   // False positives-ratio should be < 0.5 %
  BOOST_CHECK_LT(fpfn.second, 5e-3);  // False negatives-ratio should be < 0.5 %
}

BOOST_AUTO_TEST_CASE(full_polarization) {
  size_t width = 200, height = 50;
  Mask2D mask = Mask2D::MakeSetMask<false>(width, height);
  TimeFrequencyData allData;
  PolarizationEnum pols[4] = {Polarization::XX, Polarization::XY,
                              Polarization::YX, Polarization::YY};
  for (size_t i = 0; i != 4; ++i) {
    Image2D real = TestSetGenerator::MakeTestSet(3, mask, width, height),
            imag = TestSetGenerator::MakeTestSet(3, mask, width, height);
    TimeFrequencyData tfData(pols[i], Image2D::MakePtr(real),
                             Image2D::MakePtr(imag));
    if (i == 0)
      allData = tfData;
    else
      allData =
          TimeFrequencyData::MakeFromPolarizationCombination(allData, tfData);
  }

  auto fpfn = checkResult(allData, mask);
  BOOST_CHECK_LT(fpfn.first, 2e-2);   // False positives-ratio should be < 2 %
  BOOST_CHECK_LT(fpfn.second, 2e-3);  // False negatives-ratio should be < 0.2 %
}

BOOST_AUTO_TEST_SUITE_END()
