
#include <boost/test/unit_test.hpp>

#include "../../lua/default-strategy.h"
#include "../../lua/luastrategy.h"
#include "../../lua/scriptdata.h"

#include "../../algorithms/testsetgenerator.h"

using aocommon::Polarization;

BOOST_AUTO_TEST_SUITE(lua_flagnans, *boost::unit_test::label("lua"))

static void initStrategy(LuaStrategy& strategy) {
  strategy.Initialize();
  strategy.LoadText(
      "function execute(input)\n"
      "  input:flag_nans()\n"
      "end");
}

static TimeFrequencyData run(const TimeFrequencyData& data) {
  LuaStrategy strategy;
  initStrategy(strategy);
  ScriptData scriptData;
  TimeFrequencyMetaDataCPtr metaData(new TimeFrequencyMetaData());
  TimeFrequencyData runData(data);
  strategy.Execute(runData, metaData, scriptData, "execute");

  return runData;
}

BOOST_AUTO_TEST_CASE(stokesi_complex) {
  const size_t width = 3, height = 2;
  Image2DPtr real = Image2D::CreateSetImagePtr(width, height, 0.0),
             imag = Image2D::CreateSetImagePtr(width, height, 0.0);
  real->SetValue(0, 0, 1.0);
  imag->SetValue(0, 1, -1.0);
  real->SetValue(2, 0, std::numeric_limits<num_t>::quiet_NaN());
  imag->SetValue(1, 1, std::numeric_limits<num_t>::quiet_NaN());
  const TimeFrequencyData tfDataInput(Polarization::StokesI, real, imag);

  const TimeFrequencyData tfData = run(tfDataInput);

  BOOST_REQUIRE_EQUAL(tfData.MaskCount(), 1);
  Mask2DCPtr mask = tfData.GetMask(0);
  BOOST_CHECK_EQUAL(mask->ToString(),
                    "  X\n"
                    " X \n");
}

BOOST_AUTO_TEST_CASE(four_pol_real) {
  const size_t width = 4, height = 6;
  Image2DPtr realXX = Image2D::CreateSetImagePtr(width, height, 0.0),
             imagXX = Image2D::CreateSetImagePtr(width, height, 0.0),
             realXY = Image2D::CreateSetImagePtr(width, height, 0.0),
             imagXY = Image2D::CreateSetImagePtr(width, height, 0.0),
             realYX = Image2D::CreateSetImagePtr(width, height, 0.0),
             imagYX = Image2D::CreateSetImagePtr(width, height, 0.0),
             realYY = Image2D::CreateSetImagePtr(width, height, 0.0),
             imagYY = Image2D::CreateSetImagePtr(width, height, 0.0);
  realXX->SetValue(1, 1, std::numeric_limits<num_t>::quiet_NaN());
  imagXX->SetValue(2, 1, std::numeric_limits<num_t>::quiet_NaN());
  TimeFrequencyData xx(Polarization::XX, realXX, imagXX);
  realXY->SetValue(1, 2, std::numeric_limits<num_t>::quiet_NaN());
  imagXY->SetValue(2, 2, std::numeric_limits<num_t>::quiet_NaN());
  TimeFrequencyData xy(Polarization::XY, realXY, imagXY);
  realYX->SetValue(1, 3, std::numeric_limits<num_t>::quiet_NaN());
  imagYX->SetValue(2, 3, std::numeric_limits<num_t>::quiet_NaN());
  TimeFrequencyData yx(Polarization::YX, realYX, imagYX);
  realYY->SetValue(1, 4, std::numeric_limits<num_t>::quiet_NaN());
  imagYY->SetValue(2, 4, std::numeric_limits<num_t>::quiet_NaN());
  TimeFrequencyData yy(Polarization::YY, realYY, imagYY);

  const TimeFrequencyData tfDataInput =
      TimeFrequencyData::MakeFromPolarizationCombination(
          TimeFrequencyData::MakeFromPolarizationCombination(xx, xy),
          TimeFrequencyData::MakeFromPolarizationCombination(yx, yy));

  const TimeFrequencyData tfData = run(tfDataInput);

  BOOST_REQUIRE_EQUAL(tfData.MaskCount(), 4);

  BOOST_CHECK_EQUAL(tfData.GetMask(0)->ToString(),
                    "    \n"
                    " XX \n"
                    "    \n"
                    "    \n"
                    "    \n"
                    "    \n");
  BOOST_CHECK_EQUAL(tfData.GetMask(1)->ToString(),
                    "    \n"
                    "    \n"
                    " XX \n"
                    "    \n"
                    "    \n"
                    "    \n");
  BOOST_CHECK_EQUAL(tfData.GetMask(2)->ToString(),
                    "    \n"
                    "    \n"
                    "    \n"
                    " XX \n"
                    "    \n"
                    "    \n");
  BOOST_CHECK_EQUAL(tfData.GetMask(3)->ToString(),
                    "    \n"
                    "    \n"
                    "    \n"
                    "    \n"
                    " XX \n"
                    "    \n");
}

BOOST_AUTO_TEST_SUITE_END()
