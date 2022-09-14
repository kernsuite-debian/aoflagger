#include "../../quality/statisticscollection.h"
#include "../../quality/qualitytablesformatter.h"

#include <boost/test/unit_test.hpp>

#include <cmath>
#include <iomanip>

BOOST_AUTO_TEST_SUITE(statistics_collection,
                      *boost::unit_test::label("quality"))

static void AssertZero(const DefaultStatistics& statistics) {
  for (size_t i = 0; i < statistics.PolarizationCount(); ++i) {
    BOOST_CHECK_EQUAL(statistics.count[i], 0ul);
    BOOST_CHECK_EQUAL(statistics.sum[i].real(), 0.0);
    BOOST_CHECK_EQUAL(statistics.sum[i].imag(), 0.0);
    BOOST_CHECK_EQUAL(statistics.sumP2[i].real(), 0.0);
    BOOST_CHECK_EQUAL(statistics.sumP2[i].imag(), 0.0);
    BOOST_CHECK_EQUAL(statistics.dCount[i], 0ul);
    BOOST_CHECK_EQUAL(statistics.dSum[i].real(), 0.0);
    BOOST_CHECK_EQUAL(statistics.dSum[i].imag(), 0.0);
    BOOST_CHECK_EQUAL(statistics.dSumP2[i].real(), 0.0);
    BOOST_CHECK_EQUAL(statistics.dSumP2[i].imag(), 0.0);
    BOOST_CHECK_EQUAL(statistics.rfiCount[i], 0ul);
  }
}

static void AssertBasicExample(const DefaultStatistics& statistics) {
  BOOST_CHECK_EQUAL(statistics.count[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.sum->real(), 6.0);
  BOOST_CHECK_EQUAL(statistics.sum->imag(), 18.0);
  BOOST_CHECK_EQUAL(statistics.sumP2->real(), 14.0);
  BOOST_CHECK_EQUAL(statistics.sumP2->imag(), 116.0);
  BOOST_CHECK_EQUAL(statistics.dCount[0], 2ul);
  BOOST_CHECK_CLOSE(statistics.dSum->real(), 2.0 * M_SQRT1_2, 1e-5);
  BOOST_CHECK_CLOSE(statistics.dSum->imag(), 4.0 * M_SQRT1_2, 1e-5);
  // The correct values are calculated e.g. as in:
  // (2.0 * M_SQRT1_2 - 1.0 * M_SQRT1_2) ^ 2 + (3.0 * M_SQRT1_2 - 2.0 *
  // M_SQRT1_2) ^ 2 (= 1/2 + 1/2 )
  BOOST_CHECK_CLOSE(statistics.dSumP2->real(), 1.0, 1e-5);
  // (6.0 * M_SQRT1_2 - 4.0 * M_SQRT1_2) ^ 2 + (8.0 * M_SQRT1_2 - 6.0 *
  // M_SQRT1_2) ^ 2 (= 2 + 2 )
  BOOST_CHECK_CLOSE(statistics.dSumP2->imag(), 4.0, 1e-5);
  BOOST_CHECK_EQUAL(statistics.rfiCount[0], 0ul);
}

BOOST_AUTO_TEST_CASE(constructor) {
  StatisticsCollection* collection = new StatisticsCollection();
  collection->SetPolarizationCount(1);
  BOOST_CHECK_EQUAL(collection->PolarizationCount(), 1u);
  DefaultStatistics statistics(1);
  collection->GetGlobalCrossBaselineStatistics(statistics);
  AssertZero(statistics);
  delete collection;

  collection = new StatisticsCollection(1);
  BOOST_CHECK_EQUAL(collection->PolarizationCount(), 1u);
  collection->GetGlobalCrossBaselineStatistics(statistics);
  AssertZero(statistics);

  StatisticsCollection copy(*collection);
  BOOST_CHECK_EQUAL(copy.PolarizationCount(), 1u);
  copy.GetGlobalCrossBaselineStatistics(statistics);
  AssertZero(statistics);
  delete collection;
}

BOOST_AUTO_TEST_CASE(collecting) {
  StatisticsCollection collection(1);
  double frequencies[3] = {100, 101, 102};
  collection.InitializeBand(0, frequencies, 3);
  float reals[3] = {1.0, 2.0, 3.0}, imags[3] = {4.0, 6.0, 8.0};
  bool isRFI[3] = {false, false, false};
  bool isPreFlagged[3] = {false, false, false};
  collection.Add(0, 0, 0.0, 0, 0, reals, imags, isRFI, isPreFlagged, 3, 1, 1,
                 1);

  DefaultStatistics statistics(1);
  collection.GetGlobalCrossBaselineStatistics(statistics);
  AssertZero(statistics);

  collection.GetGlobalFrequencyStatistics(statistics);
  AssertZero(statistics);

  collection.GetGlobalTimeStatistics(statistics);
  AssertZero(statistics);

  collection.GetGlobalAutoBaselineStatistics(statistics);
  AssertBasicExample(statistics);

  collection.Add(0, 1, 0.0, 0, 0, reals, imags, isRFI, isPreFlagged, 3, 1, 1,
                 1);
  collection.GetGlobalCrossBaselineStatistics(statistics);
  AssertBasicExample(statistics);

  collection.GetGlobalTimeStatistics(statistics);
  AssertBasicExample(statistics);

  collection.GetGlobalFrequencyStatistics(statistics);
  BOOST_CHECK_EQUAL(statistics.count[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.sum->real(), 6.0);
  BOOST_CHECK_EQUAL(statistics.sum->imag(), 18.0);
  BOOST_CHECK_EQUAL(statistics.sumP2->real(), 14.0);
  BOOST_CHECK_EQUAL(statistics.sumP2->imag(), 116.0);
  BOOST_CHECK_EQUAL(statistics.dCount[0], 4ul);
  BOOST_CHECK_CLOSE(statistics.dSum->real(), 4.0 * M_SQRT1_2, 1e-5);
  BOOST_CHECK_CLOSE(statistics.dSum->imag(), 8.0 * M_SQRT1_2, 1e-5);
  BOOST_CHECK_CLOSE(statistics.dSumP2->real(), 2.0, 1e-5);
  BOOST_CHECK_CLOSE(statistics.dSumP2->imag(), 8.0, 1e-5);
  BOOST_CHECK_EQUAL(statistics.rfiCount[0], 0ul);

  bool flagged[3] = {true, true, true};
  collection.Add(0, 1, 0.0, 0, 0, reals, imags, flagged, isPreFlagged, 3, 1, 1,
                 1);
  collection.GetGlobalTimeStatistics(statistics);
  BOOST_CHECK_EQUAL(statistics.count[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.rfiCount[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.sum->real(), 6.0);

  collection.Add(0, 1, 0.0, 0, 0, reals, imags, flagged, flagged, 3, 1, 1, 1);
  collection.GetGlobalTimeStatistics(statistics);
  BOOST_CHECK_EQUAL(statistics.count[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.rfiCount[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.sum->real(), 6.0);
}

BOOST_AUTO_TEST_CASE(image_collecting) {
  StatisticsCollection collection(1);
  double frequencies[3] = {100, 101, 102};
  collection.InitializeBand(0, frequencies, 3);
  Image2DPtr realImg = Image2D::CreateUnsetImagePtr(1, 3),
             imagImg = Image2D::CreateUnsetImagePtr(1, 3);
  realImg->SetValue(0, 0, 1.0);
  imagImg->SetValue(0, 0, 4.0);
  realImg->SetValue(0, 1, 2.0);
  imagImg->SetValue(0, 1, 6.0);
  realImg->SetValue(0, 2, 3.0);
  imagImg->SetValue(0, 2, 8.0);
  Mask2DPtr rfiMask = Mask2D::CreateSetMaskPtr<false>(1, 3),
            preFlaggedMask = Mask2D::CreateSetMaskPtr<false>(1, 3);
  const double times[1] = {0.0};

  collection.AddImage(0, 0, times, 0, 0, realImg, imagImg, rfiMask,
                      preFlaggedMask);

  DefaultStatistics statistics(1);
  collection.GetGlobalCrossBaselineStatistics(statistics);
  AssertZero(statistics);

  collection.GetGlobalFrequencyStatistics(statistics);
  AssertZero(statistics);

  collection.GetGlobalTimeStatistics(statistics);
  AssertZero(statistics);

  collection.GetGlobalAutoBaselineStatistics(statistics);
  AssertBasicExample(statistics);

  collection.AddImage(0, 1, times, 0, 0, realImg, imagImg, rfiMask,
                      preFlaggedMask);
  collection.GetGlobalCrossBaselineStatistics(statistics);
  AssertBasicExample(statistics);

  collection.GetGlobalTimeStatistics(statistics);
  AssertBasicExample(statistics);

  collection.GetGlobalFrequencyStatistics(statistics);
  BOOST_CHECK_EQUAL(statistics.count[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.sum->real(), 6.0);
  BOOST_CHECK_EQUAL(statistics.sum->imag(), 18.0);
  BOOST_CHECK_EQUAL(statistics.sumP2->real(), 14.0);
  BOOST_CHECK_EQUAL(statistics.sumP2->imag(), 116.0);
  BOOST_CHECK_EQUAL(statistics.dCount[0], 4ul);
  BOOST_CHECK_CLOSE(statistics.dSum->real(), 4.0 * M_SQRT1_2, 1e-5);
  BOOST_CHECK_CLOSE(statistics.dSum->imag(), 8.0 * M_SQRT1_2, 1e-5);
  BOOST_CHECK_CLOSE(statistics.dSumP2->real(), 2.0, 1e-5);
  BOOST_CHECK_CLOSE(statistics.dSumP2->imag(), 8.0, 1e-5);
  BOOST_CHECK_EQUAL(statistics.rfiCount[0], 0ul);

  rfiMask->SetAll<true>();
  collection.AddImage(0, 1, times, 0, 0, realImg, imagImg, rfiMask,
                      preFlaggedMask);
  collection.GetGlobalTimeStatistics(statistics);
  BOOST_CHECK_EQUAL(statistics.count[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.rfiCount[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.sum->real(), 6.0);

  collection.AddImage(0, 1, times, 0, 0, realImg, imagImg, rfiMask, rfiMask);
  collection.GetGlobalTimeStatistics(statistics);
  BOOST_CHECK_EQUAL(statistics.count[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.rfiCount[0], 3ul);
  BOOST_CHECK_EQUAL(statistics.sum->real(), 6.0);
}

static void testCollectingImage(Image2DCPtr image, Mask2DCPtr mask,
                                size_t nTimes, size_t nFreq) {
  size_t iterations = 1, startAntenna = 1;
  StatisticsCollection collectionA(1), collectionB(1);
  std::vector<double> frequencies(nFreq), times(nTimes);
  for (size_t i = 0; i != nFreq; ++i) frequencies[i] = i + 1;
  for (size_t i = 0; i != nTimes; ++i) times[i] = i + 1;

  collectionA.InitializeBand(0, &frequencies[0], nFreq);
  collectionB.InitializeBand(0, &frequencies[0], nFreq);

  for (size_t a = startAntenna; a != iterations + startAntenna; ++a) {
    for (size_t t = 0; t < image->Width(); ++t) {
      collectionA.Add(0, a, times[t], 0, 0, image->Data() + t,
                      image->Data() + t, mask->Data() + t, mask->Data() + t,
                      nFreq, image->Stride(), mask->Stride(), mask->Stride());
    }
  }

  for (size_t a = startAntenna; a != iterations + startAntenna; ++a)
    collectionB.AddImage(0, a, &times[0], 0, 0, image, image, mask, mask);

  DefaultStatistics statA(1), statB(1);
  collectionA.GetGlobalCrossBaselineStatistics(statA);
  collectionB.GetGlobalCrossBaselineStatistics(statB);
  if (statA != statB) {
    BOOST_CHECK_EQUAL(statA.count[0], statB.count[0]);
    BOOST_CHECK_CLOSE((double)statA.sum[0].real(), (double)statB.sum[0].real(),
                      1e-5);
    BOOST_CHECK_CLOSE((double)statA.sumP2[0].real(),
                      (double)statB.sumP2[0].real(), 1e-5);
    BOOST_CHECK_EQUAL(statA.dCount[0], statB.dCount[0]);
    BOOST_CHECK_CLOSE((double)statA.dSum[0].real(),
                      (double)statB.dSum[0].real(), 1e-5);
    // 		BOOST_CHECK_CLOSE((double) statA.dSumP2[0].real(), (double)
    // statB.dSumP2[0].real(), 1e-5);
  }
}

BOOST_AUTO_TEST_CASE(comparison) {
  size_t nFreq = 500, nTimes = 2000;

  Image2DPtr image = Image2D::CreateZeroImagePtr(nTimes, nFreq);
  Mask2DPtr mask = Mask2D::CreateSetMaskPtr<false>(nTimes, nFreq);

  testCollectingImage(image, mask, nTimes, nFreq);

  image->SetAll(1.0);
  testCollectingImage(image, mask, nTimes, nFreq);

  image->SetAll(1.0);
  testCollectingImage(image, mask, nTimes, nFreq);

  for (size_t y = 0; y != nFreq; ++y) {
    for (size_t x = 0; x != nTimes; ++x) mask->SetValue(x, y, (x + y) % 2 == 0);
  }
  testCollectingImage(image, mask, nTimes, nFreq);

  mask->SetAll<false>();
  for (size_t i = 0; i < nFreq * nTimes; i += 17) {
    size_t x = i / nFreq, y = i % nFreq;
    image->SetValue(x, y, i % 3);
    if ((i % 5) == 0) mask->SetValue(x, y, true);
  }

  testCollectingImage(image, mask, nTimes, nFreq);
}

BOOST_AUTO_TEST_SUITE_END()
