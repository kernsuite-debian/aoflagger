#include "../../quality/statisticsderivator.h"
#include "../../quality/defaultstatistics.h"

#include <boost/test/unit_test.hpp>

#include <cmath>

BOOST_AUTO_TEST_SUITE(statistics_derivator, *boost::unit_test::label("quality"))

BOOST_AUTO_TEST_CASE(statistics) {
  // Our sequence: 0.0 0.0 0.0
  std::complex<long double> val = StatisticsDerivator::Variance(3, 0.0, 0.0);
  BOOST_CHECK_EQUAL(val, std::complex<long double>(0.0, 0.0));

  // Our sequence: 1.0 2.0 3.0
  val = StatisticsDerivator::Variance(3, 6.0, 14.0);
  BOOST_CHECK_CLOSE(val.real(), 1.0, 1e-5);  // (1 + 0 + 1) / (n-1) = 1.0

  // Our sequence: 1.0+4.0i 2.0+6.0i 3.0+8.0i
  DefaultStatistics statistics(1);
  statistics.count[0] = 3;
  statistics.sum[0] = std::complex<long double>(6.0, 18.0);
  statistics.sumP2[0] = std::complex<long double>(14.0, 116.0);
  val = StatisticsDerivator::GetComplexStatistic(
      QualityTablesFormatter::CountStatistic, statistics, 0);
  BOOST_CHECK_CLOSE(val.real(), 3.0, 1e-5);
  val = StatisticsDerivator::GetComplexStatistic(
      QualityTablesFormatter::SumStatistic, statistics, 0);
  BOOST_CHECK_CLOSE(val.real(), 6.0, 1e-5);
  BOOST_CHECK_CLOSE(val.imag(), 18.0, 1e-5);
  val = StatisticsDerivator::GetComplexStatistic(
      QualityTablesFormatter::SumP2Statistic, statistics, 0);
  BOOST_CHECK_CLOSE(val.real(), 14.0, 1e-5);
  BOOST_CHECK_CLOSE(val.imag(), 116.0, 1e-5);
  val = StatisticsDerivator::GetComplexStatistic(
      QualityTablesFormatter::MeanStatistic, statistics, 0);
  BOOST_CHECK_CLOSE(val.real(), 6.0 / 3.0, 1e-5);
  BOOST_CHECK_CLOSE(val.imag(), 18.0 / 3.0, 1e-5);
  val = StatisticsDerivator::GetComplexStatistic(
      QualityTablesFormatter::VarianceStatistic, statistics, 0);
  BOOST_CHECK_CLOSE(val.real(), 1.0, 1e-5);  // (1 + 0 + 1) / (n-1) = 1.0
  BOOST_CHECK_CLOSE(val.imag(), 4.0, 1e-5);  // (4 + 0 + 4) / (n-1) = 4.0
  val = StatisticsDerivator::GetComplexStatistic(
      QualityTablesFormatter::StandardDeviationStatistic, statistics, 0);
  BOOST_CHECK_CLOSE(val.real(), sqrt(2.0 / 3.0),
                    1e-5);  // sqrt((1 + 0 + 1) / n) = sqrt(2/3)
  BOOST_CHECK_CLOSE(val.imag(), sqrt(8.0 / 3.0),
                    1e-5);  // sqrt((4 + 0 + 4) / n) = sqrt(8/3)

  statistics.dCount[0] = statistics.count[0];
  statistics.dSum[0] = statistics.sum[0];
  statistics.dSumP2[0] = statistics.sumP2[0];
  val = StatisticsDerivator::GetComplexStatistic(
      QualityTablesFormatter::SignalToNoiseStatistic, statistics, 0);
  BOOST_CHECK_CLOSE(val.real(), 2.0 / sqrt(2.0 / 3.0), 1e-5);
  BOOST_CHECK_CLOSE(val.imag(), 6.0 / sqrt(8.0 / 3.0), 1e-5);
}

BOOST_AUTO_TEST_SUITE_END()
