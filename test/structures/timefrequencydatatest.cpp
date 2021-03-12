#include "../../structures/timefrequencydata.h"
#include "../../structures/image2d.h"

#include <boost/test/unit_test.hpp>

using namespace aocommon;

BOOST_AUTO_TEST_SUITE(default_strategy_speed_test,
                      *boost::unit_test::label("structures"))

BOOST_AUTO_TEST_CASE(construction) {
  TimeFrequencyData a;
  BOOST_CHECK_EQUAL(a.PolarizationCount(), size_t(0));

  Image2DPtr image = Image2D::CreateSetImagePtr(10, 10, 1.0);
  TimeFrequencyData b = TimeFrequencyData::FromLinear(
      image, image, image, image, image, image, image, image);
  BOOST_CHECK_EQUAL(b.PolarizationCount(), size_t(4));
  BOOST_CHECK_EQUAL(b.ComplexRepresentation(), TimeFrequencyData::ComplexParts);

  TimeFrequencyData c = TimeFrequencyData(Polarization::XX, image, image,
                                          Polarization::YY, image, image);
  BOOST_CHECK_EQUAL(c.PolarizationCount(), size_t(2));
  BOOST_CHECK_EQUAL(c.ComplexRepresentation(), TimeFrequencyData::ComplexParts);

  TimeFrequencyData d =
      TimeFrequencyData(TimeFrequencyData::AmplitudePart, Polarization::XX,
                        image, Polarization::YY, image);
  BOOST_CHECK_EQUAL(d.PolarizationCount(), size_t(2));
  BOOST_CHECK_EQUAL(d.ComplexRepresentation(),
                    TimeFrequencyData::AmplitudePart);

  TimeFrequencyData e = TimeFrequencyData(Polarization::RR, image, image,
                                          Polarization::LL, image, image);
  BOOST_CHECK_EQUAL(c.PolarizationCount(), size_t(2));
  BOOST_CHECK_EQUAL(c.ComplexRepresentation(), TimeFrequencyData::ComplexParts);

  TimeFrequencyData f =
      TimeFrequencyData(TimeFrequencyData::AmplitudePart, Polarization::RR,
                        image, Polarization::LL, image);
  BOOST_CHECK_EQUAL(d.PolarizationCount(), size_t(2));
  BOOST_CHECK_EQUAL(d.ComplexRepresentation(),
                    TimeFrequencyData::AmplitudePart);
}

// TODO
// BOOST_AUTO_TEST_CASE(assignment)
//{
//}

BOOST_AUTO_TEST_CASE(conversion) {
  Image2DPtr image = Image2D::CreateSetImagePtr(10, 10, 1.0);
  TimeFrequencyData data = TimeFrequencyData(Polarization::XX, image, image,
                                             Polarization::YY, image, image);
  TimeFrequencyData stokesI = data.Make(Polarization::StokesI);
  BOOST_CHECK_EQUAL(stokesI.PolarizationCount(), size_t(1));
  BOOST_CHECK_EQUAL(stokesI.ComplexRepresentation(),
                    TimeFrequencyData::ComplexParts);
  // Test whether this does not crash
  Image2DCPtr derivedImage = stokesI.GetSingleImage();

  data = TimeFrequencyData(TimeFrequencyData::AmplitudePart, Polarization::XX,
                           image, Polarization::YY, image);
  stokesI = data.Make(Polarization::StokesI);
  BOOST_CHECK_EQUAL(stokesI.PolarizationCount(), size_t(1));
  BOOST_CHECK_EQUAL(stokesI.ComplexRepresentation(),
                    TimeFrequencyData::AmplitudePart);
  // Test whether this does not crash
  derivedImage = stokesI.GetSingleImage();

  data = TimeFrequencyData(Polarization::RR, image, image, Polarization::LL,
                           image, image);
  stokesI = data.Make(Polarization::StokesI);
  BOOST_CHECK_EQUAL(stokesI.PolarizationCount(), size_t(1));
  BOOST_CHECK_EQUAL(stokesI.ComplexRepresentation(),
                    TimeFrequencyData::ComplexParts);
  // Test whether this does not crash
  derivedImage = stokesI.GetSingleImage();

  data = TimeFrequencyData(TimeFrequencyData::AmplitudePart, Polarization::RR,
                           image, Polarization::LL, image);
  stokesI = data.Make(Polarization::StokesI);
  BOOST_CHECK_EQUAL(stokesI.PolarizationCount(), size_t(1));
  BOOST_CHECK_EQUAL(stokesI.ComplexRepresentation(),
                    TimeFrequencyData::AmplitudePart);
  // Test whether this does not crash
  derivedImage = stokesI.GetSingleImage();

  TimeFrequencyData xx(TimeFrequencyData::AmplitudePart, Polarization::XX,
                       image);
  TimeFrequencyData yy(TimeFrequencyData::AmplitudePart, Polarization::YY,
                       image);
  data = TimeFrequencyData::MakeFromPolarizationCombination(xx, yy);
  BOOST_CHECK_EQUAL(data.PolarizationCount(), size_t(2));
  BOOST_CHECK_EQUAL(data.ComplexRepresentation(),
                    TimeFrequencyData::AmplitudePart);

  xx = TimeFrequencyData(Polarization::XX, image, image);
  yy = TimeFrequencyData(Polarization::YY, image, image);
  data = TimeFrequencyData::MakeFromPolarizationCombination(xx, yy);
  BOOST_CHECK_EQUAL(data.PolarizationCount(), size_t(2));
  BOOST_CHECK_EQUAL(data.ComplexRepresentation(),
                    TimeFrequencyData::ComplexParts);

  TimeFrequencyData rr(TimeFrequencyData::AmplitudePart, Polarization::XX,
                       image);
  TimeFrequencyData rl(TimeFrequencyData::AmplitudePart, Polarization::YY,
                       image);
  TimeFrequencyData lr(TimeFrequencyData::AmplitudePart, Polarization::XX,
                       image);
  TimeFrequencyData ll(TimeFrequencyData::AmplitudePart, Polarization::YY,
                       image);
  data = TimeFrequencyData::MakeFromPolarizationCombination(
      TimeFrequencyData::MakeFromPolarizationCombination(rr, ll),
      TimeFrequencyData::MakeFromPolarizationCombination(rl, lr));
  BOOST_CHECK_EQUAL(data.PolarizationCount(), size_t(4));
  BOOST_CHECK_EQUAL(data.ComplexRepresentation(),
                    TimeFrequencyData::AmplitudePart);
  stokesI = data.Make(Polarization::StokesI);
  BOOST_CHECK_EQUAL(stokesI.PolarizationCount(), size_t(1));
  BOOST_CHECK_EQUAL(stokesI.ComplexRepresentation(),
                    TimeFrequencyData::AmplitudePart);
}

BOOST_AUTO_TEST_SUITE_END()
