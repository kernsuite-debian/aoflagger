#include "../../structures/mask2d.h"

#include "../../algorithms/noisestatistics.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(noise_statistics, *boost::unit_test::label("algorithms"))

static void AssertValues(const NoiseStatistics &statistics, long unsigned count,
                         NoiseStatistics::stat_t sum,
                         NoiseStatistics::stat_t sum2,
                         NoiseStatistics::stat_t sum3,
                         NoiseStatistics::stat_t sum4) {
  BOOST_CHECK_EQUAL(statistics.Count(), count);
  BOOST_CHECK_CLOSE(statistics.Sum(), sum, 1e-5);
  BOOST_CHECK_CLOSE(statistics.Sum2(), sum2, 1e-5);
  BOOST_CHECK_CLOSE(statistics.Sum3(), sum3, 1e-5);
  BOOST_CHECK_CLOSE(statistics.Sum4(), sum4, 1e-5);
}

static void AssertValues(
    const NoiseStatistics &statistics, long unsigned count,
    NoiseStatistics::stat_t sum, NoiseStatistics::stat_t sum2,
    NoiseStatistics::stat_t sum3, NoiseStatistics::stat_t sum4,
    NoiseStatistics::stat_t mean, NoiseStatistics::stat_t moment2,
    NoiseStatistics::stat_t moment4, NoiseStatistics::stat_t stdDevEst,
    NoiseStatistics::stat_t varianceEst,
    NoiseStatistics::stat_t varianceOfVarianceEst) {
  AssertValues(statistics, count, sum, sum2, sum3, sum4);
  BOOST_CHECK_CLOSE(statistics.Mean(), mean, 1e-5);
  BOOST_CHECK_CLOSE(statistics.SecondMoment(), moment2, 1e-5);
  BOOST_CHECK_CLOSE(statistics.FourthMoment(), moment4, 1e-5);
  BOOST_CHECK_CLOSE(statistics.StdDevEstimator(), stdDevEst, 1e-5);
  BOOST_CHECK_CLOSE(statistics.VarianceEstimator(), varianceEst, 1e-5);
  BOOST_CHECK_CLOSE(statistics.VarianceOfVarianceEstimator(),
                    varianceOfVarianceEst, 1e-5);
}

static void AssertRunnable(const NoiseStatistics &statistics) {
  statistics.Count();
  statistics.Sum();
  statistics.Sum2();
  statistics.Sum3();
  statistics.Sum4();
  statistics.Mean();
  statistics.SecondMoment();
  statistics.FourthMoment();
  statistics.StdDevEstimator();
  statistics.VarianceEstimator();
  statistics.VarianceOfVarianceEstimator();
}

BOOST_AUTO_TEST_CASE(initialization) {
  // Test without initialization
  NoiseStatistics statistics;
  AssertValues(statistics, 0, 0.0, 0.0, 0.0, 0.0);
  // Some values are undefined, but should not throw an exception:
  AssertRunnable(statistics);

  // Test assignment + initialization with an array
  NoiseStatistics::Array array;
  array.push_back(1.0);
  statistics = NoiseStatistics(array);
  AssertValues(statistics, 1, 1.0, 1.0, 1.0, 1.0);
  AssertRunnable(statistics);

  // Test copy constructor
  NoiseStatistics copy(statistics);
  AssertValues(copy, 1, 1.0, 1.0, 1.0, 1.0);
  AssertRunnable(copy);
}

BOOST_AUTO_TEST_CASE(calculations) {
  NoiseStatistics::Array array;
  array.push_back(1.0);
  array.push_back(2.0);
  array.push_back(3.0);
  NoiseStatistics statistics(array);
  AssertValues(statistics, 3, 6.0, 14.0, 36.0, 98.0);
  BOOST_CHECK_CLOSE(statistics.Mean(), 2.0, 1e-6);
  BOOST_CHECK_CLOSE(statistics.SecondMoment(), 2.0 / 3.0, 1e-6);
  BOOST_CHECK_CLOSE(statistics.FourthMoment(), 2.0 / 3.0, 1e-6);
  BOOST_CHECK_CLOSE(statistics.StdDevEstimator(), 1.0, 1e-6);
  BOOST_CHECK_CLOSE(statistics.VarianceEstimator(), 1.0, 1e-6);
  AssertRunnable(statistics);

  array.clear();
  array.push_back(5.0);
  array.push_back(5.0);
  array.push_back(5.0);
  array.push_back(5.0);
  array.push_back(5.0);
  statistics = NoiseStatistics(array);
  AssertValues(statistics, 5, 25.0, 125.0, 625.0, 3125.0, 5.0, 0.0, 0.0, 0.0,
               0.0, 0.0);

  array.clear();
  array.push_back(1.0);
  array.push_back(1.0);
  array.push_back(1.0);
  array.push_back(2.0);
  array.push_back(3.0);
  statistics = NoiseStatistics(array);
  AssertValues(statistics, 5, 8.0, 16.0, 38.0, 100.0, 1.6, 0.64, 0.8512,
               sqrt(0.8), 0.8, 0.12928);

  array.clear();
  array.push_back(3.0);
  array.push_back(1.0);
  array.push_back(3.0);
  array.push_back(1.0);
  array.push_back(3.0);
  array.push_back(1.0);
  statistics = NoiseStatistics(array);
  AssertValues(statistics, 6, 12.0, 30.0, 84.0, 246.0, 2.0, 1.0, 1.0, sqrt(1.2),
               1.2, 2.0 / 30.0);
}

BOOST_AUTO_TEST_CASE(add_values) {
  NoiseStatistics statistics;

  NoiseStatistics::Array array;
  array.push_back(1.0);
  array.push_back(2.0);
  array.push_back(3.0);
  statistics.Add(array);
  AssertValues(statistics, 3, 6.0, 14.0, 36.0, 98.0);
  BOOST_CHECK_CLOSE(statistics.Mean(), 2.0, 1e-5);
  BOOST_CHECK_CLOSE(statistics.SecondMoment(), 2.0 / 3.0, 1e-5);
  BOOST_CHECK_CLOSE(statistics.FourthMoment(), 2.0 / 3.0, 1e-5);
  BOOST_CHECK_CLOSE(statistics.VarianceEstimator(), 1.0, 1e-5);
  AssertRunnable(statistics);

  array.clear();
  array.push_back(1.0);
  array.push_back(1.0);
  statistics.Add(array);
  AssertValues(statistics, 5, 8.0, 16.0, 38.0, 100.0, 1.6, 0.64, 0.8512,
               sqrt(0.8), 0.8, 0.12928);

  array.clear();
  array.push_back(5.0);
  NoiseStatistics numberFive = NoiseStatistics(array);

  statistics = NoiseStatistics();
  statistics.Add(numberFive);
  statistics.Add(numberFive);
  statistics.Add(numberFive);
  statistics.Add(numberFive);
  statistics.Add(numberFive);
  AssertValues(statistics, 5, 25.0, 125.0, 625.0, 3125.0, 5.0, 0.0, 0.0, 0.0,
               0.0, 0.0);

  array.clear();
  array.push_back(3.0);
  array.push_back(3.0);
  array.push_back(3.0);
  NoiseStatistics partA = NoiseStatistics(array);

  array.clear();
  array.push_back(1.0);
  array.push_back(1.0);
  array.push_back(1.0);
  NoiseStatistics partB = NoiseStatistics(array);
  statistics = NoiseStatistics();
  statistics.Add(partA);
  statistics.Add(partB);
  AssertValues(statistics, 6, 12.0, 30.0, 84.0, 246.0, 2.0, 1.0, 1.0, sqrt(1.2),
               1.2, 2.0 / 30.0);
}

BOOST_AUTO_TEST_SUITE_END()
