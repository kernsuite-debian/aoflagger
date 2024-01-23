#include "../../structures/timefrequencydata.h"
#include "../../structures/image2d.h"

#include <boost/test/unit_test.hpp>

using aocommon::Polarization;
using aocommon::PolarizationEnum;

namespace {

constexpr PolarizationEnum kPolarizations[] = {Polarization::XX,
                                               Polarization::YY};

/**
 * Creates a complex TimeFrequencyData object with two polarizations and given
 * size.
 * @param input should be a vector of size 4, where each element specifies (in
 * order) the real and imaginary xx and real and imaginary yy values that are
 * used to construct the images.
 */
TimeFrequencyData MakeComplexTestData(
    size_t width, size_t height,
    std::vector<std::initializer_list<num_t>> input) {
  BOOST_REQUIRE_EQUAL(input.size(), 2 * std::size(kPolarizations));
  Image2DPtr real_xx = Image2D::MakePtr(width, height, input[0]);
  Image2DPtr imag_xx = Image2D::MakePtr(width, height, input[1]);
  Image2DPtr real_yy = Image2D::MakePtr(width, height, input[2]);
  Image2DPtr imag_yy = Image2D::MakePtr(width, height, input[3]);
  const Image2DPtr lhs_reals[] = {real_xx, real_yy};
  const Image2DPtr lhs_imags[] = {imag_xx, imag_yy};
  return TimeFrequencyData(kPolarizations, std::size(kPolarizations), lhs_reals,
                           lhs_imags);
}
/**
 * Creates a real TimeFrequencyData object with two polarizations and given
 * size.
 * @param input is of size 2, specifies (in order) the xx and yy values that are
 * used to construct the images.
 */
TimeFrequencyData MakeRealTestData(
    size_t width, size_t height,
    std::vector<std::initializer_list<num_t>> input) {
  constexpr PolarizationEnum kPolarizations[] = {Polarization::XX,
                                                 Polarization::YY};
  BOOST_REQUIRE_EQUAL(input.size(), std::size(kPolarizations));
  Image2DPtr xx = Image2D::MakePtr(width, height, input[0]);
  Image2DPtr yy = Image2D::MakePtr(width, height, input[1]);
  const Image2DCPtr images[] = {xx, yy};
  return TimeFrequencyData(TimeFrequencyData::RealPart, kPolarizations,
                           std::size(kPolarizations), images);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(time_frequency_data_operations,
                      *boost::unit_test::label("structures"))

BOOST_AUTO_TEST_CASE(to_complex_vector) {
  constexpr size_t kWidth = 2;
  constexpr size_t kHeight = 3;
  Image2DPtr real_xx = Image2D::CreateSetImagePtr(kWidth, kHeight, 1.0);
  real_xx->SetValue(1, 2, 0.0);
  Image2DPtr imag_xx = Image2D::CreateSetImagePtr(kWidth, kHeight, 5.0);
  imag_xx->SetValue(1, 2, 8.0);
  Image2DPtr real_yy = Image2D::CreateSetImagePtr(kWidth, kHeight, 3.0);
  real_yy->SetValue(1, 2, 4.0);
  Image2DPtr imag_yy = Image2D::CreateSetImagePtr(kWidth, kHeight, 7.0);
  imag_yy->SetValue(1, 2, 12.0);
  const PolarizationEnum polarizations[] = {Polarization::XX, Polarization::YY};
  const Image2DPtr reals[] = {real_xx, real_yy};
  const Image2DPtr imags[] = {imag_xx, imag_yy};
  const TimeFrequencyData data(polarizations, 2, reals, imags);
  const std::vector<std::complex<num_t>> vector = ToComplexVector(data);
  const std::complex<num_t> reference[] = {
      {1.0, 5.0}, {1.0, 5.0}, {1.0, 5.0}, {1.0, 5.0}, {1.0, 5.0}, {0.0, 8.0},
      {3.0, 7.0}, {3.0, 7.0}, {3.0, 7.0}, {3.0, 7.0}, {3.0, 7.0}, {4.0, 12.0}};
  BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(reference), std::end(reference),
                                vector.begin(), vector.end());
}

BOOST_AUTO_TEST_CASE(element_wise_division_exceptions) {
  constexpr size_t kWidth = 1;
  constexpr size_t kHeight = 2;

  const TimeFrequencyData real =
      MakeRealTestData(kWidth, kHeight, {{2.0f, 1.0f}, {2.0f, 3.0f}});
  const TimeFrequencyData complex = MakeComplexTestData(
      kWidth, kHeight,
      {{8.0f, 9.0f}, {4.0f, 6.0f}, {-12.0f, 1.0f}, {12.0f, 2.0f}});
  const TimeFrequencyData empty;
  const TimeFrequencyData wrong_size = MakeRealTestData(1, 1, {{1.0f}, {2.0f}});
  Image2DPtr image = Image2D::MakePtr(kWidth, kHeight, 1.0f);
  const TimeFrequencyData wrong_polarization(TimeFrequencyData::RealPart,
                                             Polarization::XX, image);

  BOOST_CHECK_THROW(ElementWiseDivide(real, complex), std::runtime_error);
  BOOST_CHECK_THROW(ElementWiseDivide(complex, real), std::runtime_error);
  BOOST_CHECK_THROW(ElementWiseDivide(real, empty), std::runtime_error);
  BOOST_CHECK_THROW(ElementWiseDivide(empty, real), std::runtime_error);
  BOOST_CHECK_THROW(ElementWiseDivide(real, wrong_size), std::runtime_error);
  BOOST_CHECK_THROW(ElementWiseDivide(wrong_size, real), std::runtime_error);
  BOOST_CHECK_THROW(ElementWiseDivide(real, wrong_polarization),
                    std::runtime_error);
  BOOST_CHECK_THROW(ElementWiseDivide(wrong_polarization, real),
                    std::runtime_error);
}

BOOST_AUTO_TEST_CASE(empty_operations) {
  const TimeFrequencyData empty;
  BOOST_CHECK(ElementWiseDivide(empty, empty).IsEmpty());
  BOOST_CHECK(ElementWiseNorm(empty).IsEmpty());
  BOOST_CHECK(ElementWiseSqrt(empty).IsEmpty());
}

BOOST_AUTO_TEST_CASE(element_wise_division_complex) {
  constexpr size_t kWidth = 1;
  constexpr size_t kHeight = 2;

  const TimeFrequencyData lhs = MakeComplexTestData(
      kWidth, kHeight,
      {{8.0f, 9.0f}, {4.0f, 6.0f}, {-12.0f, 1.0f}, {12.0f, 2.0f}});
  const TimeFrequencyData rhs = MakeComplexTestData(
      kWidth, kHeight,
      {{2.0f, 1.0f}, {2.0f, 3.0f}, {3.0f, 1.0f}, {6.0f, 0.0f}});

  const TimeFrequencyData result = ElementWiseDivide(lhs, rhs);

  BOOST_REQUIRE_EQUAL(result.PolarizationCount(), std::size(kPolarizations));
  BOOST_REQUIRE(result.ComplexRepresentation() ==
                TimeFrequencyData::ComplexParts);
  const TimeFrequencyData xx = result.MakeFromPolarizationIndex(0);
  const TimeFrequencyData yy = result.MakeFromPolarizationIndex(1);
  // (8+4i) / (2+2i) = 3 - 1i
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 0), 3.0f, 1e-4);
  BOOST_CHECK_CLOSE(xx.GetImage(1)->Value(0, 0), -1.0f, 1e-4);
  // (9+6i) / (1+3i) = 2.7 - 2.1i
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 1), 2.7f, 1e-4);
  BOOST_CHECK_CLOSE(xx.GetImage(1)->Value(0, 1), -2.1f, 1e-4);
  // (-12+12i) / (3+6i) = 0.8 + 2.4i
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 0), 0.8f, 1e-4);
  BOOST_CHECK_CLOSE(yy.GetImage(1)->Value(0, 0), 2.4f, 1e-4);
  // (1+2i) / (1+0i) = 1 + 2i
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 1), 1.0f, 1e-4);
  BOOST_CHECK_CLOSE(yy.GetImage(1)->Value(0, 1), 2.0f, 1e-4);
}

BOOST_AUTO_TEST_CASE(element_wise_division_real) {
  constexpr size_t kWidth = 1;
  constexpr size_t kHeight = 2;

  const TimeFrequencyData lhs =
      MakeRealTestData(kWidth, kHeight, {{8.0f, 9.0f}, {12.0f, 1.0f}});
  const TimeFrequencyData rhs =
      MakeRealTestData(kWidth, kHeight, {{2.0f, 1.0f}, {4.0f, 1.0f}});
  const TimeFrequencyData result = ElementWiseDivide(lhs, rhs);
  BOOST_REQUIRE_EQUAL(result.PolarizationCount(), std::size(kPolarizations));
  BOOST_REQUIRE(result.ComplexRepresentation() == TimeFrequencyData::RealPart);
  const TimeFrequencyData xx = result.MakeFromPolarizationIndex(0);
  const TimeFrequencyData yy = result.MakeFromPolarizationIndex(1);

  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 0), 4.0f, 1e-4);  // (8 / 2)
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 1), 9.0f, 1e-4);  // (9 / 1)
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 0), 3.0f, 1e-4);  // (12 / 4)
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 1), 1.0f, 1e-4);  // (1 / 1)
}

BOOST_AUTO_TEST_CASE(element_wise_norm_complex) {
  constexpr size_t kWidth = 1;
  constexpr size_t kHeight = 2;

  const TimeFrequencyData input = MakeComplexTestData(
      kWidth, kHeight,
      {{0.0f, -2.0f}, {-1.0f, 3.0f}, {4.0f, 5.0f}, {-6.0f, -7.0f}});

  const TimeFrequencyData result = ElementWiseNorm(input);

  BOOST_REQUIRE_EQUAL(result.PolarizationCount(), std::size(kPolarizations));
  BOOST_REQUIRE(result.ComplexRepresentation() ==
                TimeFrequencyData::AmplitudePart);
  const TimeFrequencyData xx = result.MakeFromPolarizationIndex(0);
  const TimeFrequencyData yy = result.MakeFromPolarizationIndex(1);
  // norm(0 - i) = 1
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 0), 1.0f, 1e-4);
  // norm(-2 + 3i) = 13
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 1), 13.0f, 1e-4);
  // norm(4 - 6i) = 52
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 0), 52.0f, 1e-4);
  // norm(5 - 7i) = 74
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 1), 74.0f, 1e-4);
}

BOOST_AUTO_TEST_CASE(element_wise_norm_real) {
  constexpr size_t kWidth = 1;
  constexpr size_t kHeight = 2;

  const TimeFrequencyData input = MakeRealTestData(kWidth, kHeight,
                                                   {
                                                       {0.0f, -2.0f},
                                                       {-1.0f, 3.0f},
                                                   });

  const TimeFrequencyData result = ElementWiseNorm(input);

  BOOST_REQUIRE_EQUAL(result.PolarizationCount(), std::size(kPolarizations));
  BOOST_REQUIRE(result.ComplexRepresentation() == TimeFrequencyData::RealPart);
  const TimeFrequencyData xx = result.MakeFromPolarizationIndex(0);
  const TimeFrequencyData yy = result.MakeFromPolarizationIndex(1);
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 0), 0.0f, 1e-4);
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 1), 4.0f, 1e-4);
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 0), 1.0f, 1e-4);
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 1), 9.0f, 1e-4);
}

BOOST_AUTO_TEST_CASE(element_wise_sqrt_complex) {
  constexpr size_t kWidth = 1;
  constexpr size_t kHeight = 2;

  const TimeFrequencyData input = MakeComplexTestData(
      kWidth, kHeight,
      {{-16.0f, 3.0f}, {30.0f, -4.0f}, {0.0f, -8.0f}, {2.0f, 6.0f}});

  const TimeFrequencyData result = ElementWiseSqrt(input);

  BOOST_REQUIRE_EQUAL(result.PolarizationCount(), std::size(kPolarizations));
  BOOST_REQUIRE(result.ComplexRepresentation() ==
                TimeFrequencyData::ComplexParts);
  const TimeFrequencyData xx = result.MakeFromPolarizationIndex(0);
  const TimeFrequencyData yy = result.MakeFromPolarizationIndex(1);
  // sqrt(-16 + 30i) = 3 + 5i
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 0), 3.0f, 1e-4);
  BOOST_CHECK_CLOSE(xx.GetImage(1)->Value(0, 0), 5.0f, 1e-4);
  // sqrt(3 - 4i) = 2 - i
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 1), 2.0f, 1e-4);
  BOOST_CHECK_CLOSE(xx.GetImage(1)->Value(0, 1), -1.0f, 1e-4);
  // sqrt(0 + 2i) = 1 + i
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 0), 1.0f, 1e-4);
  BOOST_CHECK_CLOSE(yy.GetImage(1)->Value(0, 0), 1.0f, 1e-4);
  // sqrt(-8 + 6i) = 1 + 3i
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 1), 1.0f, 1e-4);
  BOOST_CHECK_CLOSE(yy.GetImage(1)->Value(0, 1), 3.0f, 1e-4);
}

BOOST_AUTO_TEST_CASE(element_wise_sqrt_real) {
  constexpr size_t kWidth = 1;
  constexpr size_t kHeight = 2;

  const TimeFrequencyData input =
      MakeRealTestData(kWidth, kHeight, {{16.0f, 25.0f}, {36.0f, 49.0f}});

  const TimeFrequencyData result = ElementWiseSqrt(input);

  BOOST_REQUIRE_EQUAL(result.PolarizationCount(), std::size(kPolarizations));
  BOOST_REQUIRE(result.ComplexRepresentation() == TimeFrequencyData::RealPart);
  const TimeFrequencyData xx = result.MakeFromPolarizationIndex(0);
  const TimeFrequencyData yy = result.MakeFromPolarizationIndex(1);
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 0), 4.0f, 1e-4);
  BOOST_CHECK_CLOSE(xx.GetImage(0)->Value(0, 1), 5.0f, 1e-4);
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 0), 6.0f, 1e-4);
  BOOST_CHECK_CLOSE(yy.GetImage(0)->Value(0, 1), 7.0f, 1e-4);
}

BOOST_AUTO_TEST_SUITE_END()
