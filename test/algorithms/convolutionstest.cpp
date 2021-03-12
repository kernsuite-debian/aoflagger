#include "../../algorithms/convolutions.h"

#include <boost/test/unit_test.hpp>

#include <cmath>

BOOST_AUTO_TEST_SUITE(convolutions, *boost::unit_test::label("algorithms"))

BOOST_AUTO_TEST_CASE(one_dimensional_convolution,
                     *boost::unit_test::tolerance(num_t(1e-5))) {
  // Remember that OneDimensionalConvolutionBorderInterp assumes that sumover
  // kernel == 1, otherwise we have to multiply the output with sumover kernel.
  num_t data1[3] = {0.0, 1.0, 2.0};
  num_t kernel1[1] = {1.0};
  Convolutions::OneDimensionalConvolutionBorderInterp(data1, 3, kernel1, 1);
  BOOST_CHECK_CLOSE(data1[0], 0.0, 1e-6);
  BOOST_CHECK_CLOSE(data1[1], 1.0, 1e-6);
  BOOST_CHECK_CLOSE(data1[2], 2.0, 1e-6);

  num_t kernel2[1] = {0.0};
  Convolutions::OneDimensionalConvolutionBorderInterp(data1, 3, kernel2, 1);

  num_t data3[4] = {0.0, 1.0, 2.0, 3.0};
  num_t kernel3[1] = {2.0};
  Convolutions::OneDimensionalConvolutionBorderInterp(data3, 4, kernel3, 1);
  BOOST_TEST(data3[0] == 0.0);
  BOOST_TEST(data3[1] == 1.0);
  BOOST_TEST(data3[2] == 2.0);
  BOOST_TEST(data3[3] == 3.0);

  num_t kernel4[2] = {0.0, 1.0};
  Convolutions::OneDimensionalConvolutionBorderInterp(data3, 4, kernel4, 2);
  BOOST_TEST(data3[0] == 0.0);
  BOOST_TEST(data3[1] == 1.0);
  BOOST_TEST(data3[2] == 2.0);
  BOOST_TEST(data3[3] == 3.0);

  num_t kernel5[2] = {1.0, 1.0};
  Convolutions::OneDimensionalConvolutionBorderInterp(data3, 4, kernel5, 2);
  BOOST_TEST(data3[0] == 0.0);
  BOOST_TEST(data3[1] == 0.5);
  BOOST_TEST(data3[2] == 1.5);
  BOOST_TEST(data3[3] == 2.5);

  num_t data6[4] = {0.0, 1.0, 2.0, 3.0};
  num_t kernel6[3] = {1.0, 1.0, 1.0};
  Convolutions::OneDimensionalConvolutionBorderInterp(data6, 4, kernel6, 3);
  num_t expected6[4] = {0.5, 1.0, 2.0, 2.5};
  BOOST_TEST(data6 == expected6, boost::test_tools::per_element());

  num_t data7[6] = {0.0, 0.0, 1.0, 2.0, 3.0, 0.0};
  num_t kernel7[3] = {1.0, 1.0, 1.0};
  Convolutions::OneDimensionalConvolutionBorderInterp(data7, 6, kernel7, 3);
  num_t expected7[6] = {0.0, 1.0 / 3.0, 1.0, 2.0, 5.0 / 3.0, 1.5};
  BOOST_TEST(data7 == expected7, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(one_dimensional_sinc_convolution,
                     *boost::unit_test::tolerance(num_t(1e-5))) {
  num_t data1[1] = {1.0};
  Convolutions::OneDimensionalSincConvolution(data1, 1, 1.0);
  const num_t expected1[1] = {1.0};
  BOOST_TEST(data1[0] == expected1[0]);

  Convolutions::OneDimensionalSincConvolution(data1, 0, 1.0);

  num_t data2[2] = {1.0, 1.0};
  Convolutions::OneDimensionalSincConvolution(data2, 2, 0.25);
  // const num_t expected2[2] = { 1.0, 1.0 };
  // AssertValues(this, data2, expected2, 2);
  BOOST_TEST(data2[0] == data2[1]);

  num_t data3[3] = {1.0, 1.0, 1.0};
  Convolutions::OneDimensionalSincConvolution(data3, 3, 0.25);
  BOOST_TEST(data3[0] == data3[2]);

  num_t data4[4] = {1.0, 1.0, 1.0, 1.0};
  Convolutions::OneDimensionalSincConvolution(data4, 4, 0.25);
  BOOST_TEST(data4[0] == data4[3]);
  BOOST_TEST(data4[1] == data4[2]);

  const num_t sizes5[6] = {0.01, 0.1, 1.0, 3.14, 10.0 / 3.0, 100.0};
  for (unsigned i = 0; i < 6; ++i) {
    num_t data5[5] = {0.0, 0.0, 1.0, 0.0, 0.0};
    Convolutions::OneDimensionalSincConvolution(data5, 5, sizes5[i]);
    BOOST_TEST(data5[0] == data5[4]);
    BOOST_TEST(data5[1] == data5[3]);
  }

  num_t data6[100];
  data6[0] = 1;
  for (unsigned i = 1; i < 100; ++i) data6[i] = 0;
  // Convolution with sinc frequency 0.25 will produce a low-pass filter
  // that filters any frequency > 0.25 Hz. The sinc will therefore have
  // maxima on index 1, 5, 9, ..., minima on index 3, 7, 11, .. and zero's
  // on 2, 4, 6, 8, ... .
  Convolutions::OneDimensionalSincConvolution(data6, 100, 0.25);

  // Check whether maxima decrease
  for (unsigned i = 1; i < 96; i += 4) {
    BOOST_CHECK_LT(data6[i + 4], data6[i]);
  }

  // Check whether minima increase. The border value (i=95) is not tested,
  // because of the normalization it is actually larger, which is ok.
  for (unsigned i = 3; i < 94; i += 4) {
    BOOST_CHECK_GT(data6[i + 4], data6[i]);
  }

  // Check zero points
  for (unsigned i = 2; i < 100; i += 2) {
    BOOST_TEST(data6[i] == 0.0f);
  }

  // Test whether a low-pass filter attenuates a 10.000 sample
  // high-frequency chirp signal with various levels.
  num_t data7[10000];
  for (unsigned i = 0; i < 10000; i += 2) {
    data7[i] = -1;
    data7[i + 1] = 1;
  }
  Convolutions::OneDimensionalSincConvolution(data7, 10000, 0.25);
  for (unsigned i = 10; i < 9990; ++i) {
    BOOST_CHECK_LT(std::abs(data7[i]), 0.1);
  }
  for (unsigned i = 100; i < 9900; ++i) {
    BOOST_CHECK_LT(std::abs(data7[i]), 0.01);
  }
  for (unsigned i = 1000; i < 9000; ++i) {
    BOOST_CHECK_LT(std::abs(data7[i]), 0.001);
  }

  // Test whether a low-pass filter does not attenuate a 10.000 sample
  // low-frequency chirp signal with various levels.
  num_t data8[10000];
  for (unsigned i = 0; i < 10000; ++i) {
    data8[i] = sin((num_t)i / (10.0 * 2 * M_PIn));
  }
  Convolutions::OneDimensionalSincConvolution(data8, 10000, 0.25);
  for (unsigned i = 10; i < 9950; ++i) {
    num_t val = sin((num_t)i / (10.0 * 2 * M_PIn));
    BOOST_CHECK_LT(std::abs(data8[i] - val), 0.5);
  }
  for (unsigned i = 100; i < 9900; ++i) {
    num_t val = sin((num_t)i / (10.0 * 2 * M_PIn));
    BOOST_CHECK_LT(std::abs(data8[i] - val), 0.1);
  }
  for (unsigned i = 1000; i < 9000; ++i) {
    num_t val = sin((num_t)i / (10.0 * 2 * M_PIn));
    BOOST_CHECK_LT(std::abs(data8[i] - val), 0.01);
  }
}

BOOST_AUTO_TEST_SUITE_END()
