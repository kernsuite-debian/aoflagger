#include "../../algorithms/medianwindow.h"

#include "../../util/stopwatch.h"

#include <iostream>
#include <limits>
#include <random>
#include <vector>
#include <type_traits>

#include <boost/test/unit_test.hpp>

// The result of the test have been validated with Octave
// https://www.mathworks.com/help/matlab/ref/movmedian.html

static SampleRow CreateSamples(const std::vector<num_t>& values) {
  SampleRow result{values.size()};
  for (size_t i = 0; i < values.size(); ++i) {
    result.SetValue(i, values[i]);
  }
  return result;
}

BOOST_AUTO_TEST_SUITE(meanwindow, *boost::unit_test::label("algorithms") *
                                      boost::unit_test::tolerance(num_t(1e-6)))

// movmedian([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], 3)
BOOST_AUTO_TEST_CASE(median_num_elements_even_window_size_odd) {
  SampleRow samples{
      CreateSamples(std::vector<num_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10})};
  algorithms::MedianWindow<num_t>::SubtractMedian(samples, 3);

  BOOST_TEST(samples.Value(0) == 1 - 1.5);
  BOOST_TEST(samples.Value(1) == 2 - 2);
  BOOST_TEST(samples.Value(2) == 3 - 3);
  BOOST_TEST(samples.Value(3) == 4 - 4);
  BOOST_TEST(samples.Value(4) == 5 - 5);
  BOOST_TEST(samples.Value(5) == 6 - 6);
  BOOST_TEST(samples.Value(6) == 7 - 7);
  BOOST_TEST(samples.Value(7) == 8 - 8);
  BOOST_TEST(samples.Value(8) == 9 - 9);
  BOOST_TEST(samples.Value(9) == 10 - 9.5);
}

// movmedian([1, 2, 3, 4, 5, 6, 7, 8, 9], 3)
BOOST_AUTO_TEST_CASE(median_num_elements_odd_window_size_odd) {
  SampleRow samples{
      CreateSamples(std::vector<num_t>{1, 2, 3, 4, 5, 6, 7, 8, 9})};
  algorithms::MedianWindow<num_t>::SubtractMedian(samples, 3);
  BOOST_TEST(samples.Value(0) == 1 - 1.5);
  BOOST_TEST(samples.Value(1) == 2 - 2);
  BOOST_TEST(samples.Value(2) == 3 - 3);
  BOOST_TEST(samples.Value(3) == 4 - 4);
  BOOST_TEST(samples.Value(4) == 5 - 5);
  BOOST_TEST(samples.Value(5) == 6 - 6);
  BOOST_TEST(samples.Value(6) == 7 - 7);
  BOOST_TEST(samples.Value(7) == 8 - 8);
  BOOST_TEST(samples.Value(8) == 9 - 8.5);
}

// movmedian([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], 4)
BOOST_AUTO_TEST_CASE(median_num_elements_even_window_size_even) {
  SampleRow samples{
      CreateSamples(std::vector<num_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10})};
  algorithms::MedianWindow<num_t>::SubtractMedian(samples, 4);
  BOOST_TEST(samples.Value(0) == 1 - 1.5);
  BOOST_TEST(samples.Value(1) == 2 - 2);
  BOOST_TEST(samples.Value(2) == 3 - 2.5);
  BOOST_TEST(samples.Value(3) == 4 - 3.5);
  BOOST_TEST(samples.Value(4) == 5 - 4.5);
  BOOST_TEST(samples.Value(5) == 6 - 5.5);
  BOOST_TEST(samples.Value(6) == 7 - 6.5);
  BOOST_TEST(samples.Value(7) == 8 - 7.5);
  BOOST_TEST(samples.Value(8) == 9 - 8.5);
  BOOST_TEST(samples.Value(9) == 10 - 9);
}

// movmedian([1, 2, 3, 4, 5, 6, 7, 8, 9], 4)
BOOST_AUTO_TEST_CASE(median_num_elements_odd_window_size_even) {
  SampleRow samples{
      CreateSamples(std::vector<num_t>{1, 2, 3, 4, 5, 6, 7, 8, 9})};
  algorithms::MedianWindow<num_t>::SubtractMedian(samples, 4);
  BOOST_TEST(samples.Value(0) == 1 - 1.5);
  BOOST_TEST(samples.Value(1) == 2 - 2);
  BOOST_TEST(samples.Value(2) == 3 - 2.5);
  BOOST_TEST(samples.Value(3) == 4 - 3.5);
  BOOST_TEST(samples.Value(4) == 5 - 4.5);
  BOOST_TEST(samples.Value(5) == 6 - 5.5);
  BOOST_TEST(samples.Value(6) == 7 - 6.5);
  BOOST_TEST(samples.Value(7) == 8 - 7.5);
  BOOST_TEST(samples.Value(8) == 9 - 8);
}

BOOST_AUTO_TEST_CASE(median_non_finite_elements) {
  SampleRow samples{CreateSamples(
      std::vector<num_t>{1, 2, 3, -std::numeric_limits<num_t>::infinity(), 4, 5,
                         6, std::numeric_limits<num_t>::infinity(), 7, 8, 9,
                         std::numeric_limits<num_t>::quiet_NaN(), 10, 11, 12})};

  algorithms::MedianWindow<num_t>::SubtractMedian(samples, 3);
  BOOST_TEST(samples.Value(0) == 1 - 1.5);
  BOOST_TEST(samples.Value(1) == 2 - 2);
  BOOST_TEST(samples.Value(2) == 3 - 2.5);
  BOOST_CHECK(samples.Value(3) == -std::numeric_limits<num_t>::infinity());
  BOOST_TEST(samples.Value(4) == 4 - 4.5);
  BOOST_TEST(samples.Value(5) == 5 - 5);
  BOOST_TEST(samples.Value(6) == 6 - 5.5);
  BOOST_CHECK(samples.Value(7) == std::numeric_limits<num_t>::infinity());
  BOOST_TEST(samples.Value(8) == 7 - 7.5);
  BOOST_TEST(samples.Value(9) == 8 - 8);
  BOOST_TEST(samples.Value(10) == 9 - 8.5);
  BOOST_CHECK(std::isnan(samples.Value(11)));
  BOOST_TEST(samples.Value(12) == 10 - 10.5);
  BOOST_TEST(samples.Value(13) == 11 - 11);
  BOOST_TEST(samples.Value(14) == 12 - 11.5);
}

static std::vector<num_t> MakeData(size_t size) {
  std::random_device seed;
  std::mt19937 generator(seed());
  std::uniform_int_distribution<std::conditional<
      sizeof(num_t) == sizeof(uint32_t), uint32_t, uint64_t>::type>
      distribution;

  std::vector<num_t> result;
  std::generate_n(std::back_inserter(result), size, [&] {
#if defined(__has_builtin)
#if __has_builtin(__builtin_bit_cast)
#define HAS_BUILTIN_BIT_CAST
#endif
#endif
#ifdef HAS_BUILTIN_BIT_CAST
    // This is the same implementation as C++20's std::bit_cast.
    return __builtin_bit_cast(num_t, distribution(generator));
#else
	const auto value = distribution(generator);
	num_t result;
	memcpy(&result, &value, sizeof(result));
	return result;
#endif
  });
  return result;
}

BOOST_AUTO_TEST_CASE(speed, *boost::unit_test::disabled()) {
  // The size of samples is choosen so the tests are done in a reasonable time.
  const size_t kSize = 4 * 1024 * 1024;
  const SampleRow samples{CreateSamples(MakeData(kSize))};

  std::cout << "SubtractMedian for " << kSize << " elements\n";
  for (int i = 1; i < 16; ++i) {
    // An odd sized window has the median value in the middle of the window.
    const int size = (2 << i) - 1;
    SampleRow s = samples;

    Stopwatch stopwatch(true);
    algorithms::MedianWindow<num_t>::SubtractMedian(s, size);
    stopwatch.Pause();
    std::cout << "\tWindow " << size << '\t' << stopwatch.ToString() << '\n';
  }
}

BOOST_AUTO_TEST_SUITE_END()
