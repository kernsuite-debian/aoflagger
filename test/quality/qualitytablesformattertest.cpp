
#include "../../quality/qualitytablesformatter.h"
#include "../../quality/statisticalvalue.h"

#include <casacore/tables/Tables/Table.h>
#include <casacore/tables/Tables/SetupNewTab.h>
#include <casacore/tables/Tables/ScaColDesc.h>

#include <boost/test/unit_test.hpp>

struct Fixture {
  Fixture() {
    casacore::TableDesc tableDesc("MAIN_TABLE", "1.0",
                                  casacore::TableDesc::Scratch);
    tableDesc.addColumn(casacore::ScalarColumnDesc<int>("TEST"));
    casacore::SetupNewTable mainTableSetup("QualityTest.MS", tableDesc,
                                           casacore::Table::New);
    casacore::Table mainOutputTable(mainTableSetup);
  }
  ~Fixture() { casacore::Table::deleteTable("QualityTest.MS"); }
};

BOOST_AUTO_TEST_SUITE(quality_tables_formatter,
                      *boost::unit_test::label("quality") *
                          boost::unit_test::fixture<Fixture>())

BOOST_AUTO_TEST_CASE(constructor) {
  QualityTablesFormatter qd("QualityTest.MS");
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(table_exists) {
  QualityTablesFormatter qd("QualityTest.MS");
  // undefined answer, but should not crash.
  qd.TableExists(QualityTablesFormatter::KindNameTable);
  qd.TableExists(QualityTablesFormatter::TimeStatisticTable);
  qd.TableExists(QualityTablesFormatter::FrequencyStatisticTable);
  qd.TableExists(QualityTablesFormatter::BaselineStatisticTable);
  qd.TableExists(QualityTablesFormatter::BaselineTimeStatisticTable);

  qd.RemoveAllQualityTables();
  BOOST_CHECK(!qd.TableExists(QualityTablesFormatter::KindNameTable));
  BOOST_CHECK(!qd.TableExists(QualityTablesFormatter::TimeStatisticTable));
  BOOST_CHECK(!qd.TableExists(QualityTablesFormatter::FrequencyStatisticTable));
  BOOST_CHECK(!qd.TableExists(QualityTablesFormatter::BaselineStatisticTable));
  BOOST_CHECK(
      !qd.TableExists(QualityTablesFormatter::BaselineTimeStatisticTable));
}

BOOST_AUTO_TEST_CASE(table_initialization) {
  QualityTablesFormatter qd("QualityTest.MS");

  qd.RemoveAllQualityTables();

  enum QualityTablesFormatter::QualityTable tables[5] = {
      QualityTablesFormatter::KindNameTable,
      QualityTablesFormatter::TimeStatisticTable,
      QualityTablesFormatter::FrequencyStatisticTable,
      QualityTablesFormatter::BaselineStatisticTable,
      QualityTablesFormatter::BaselineTimeStatisticTable};
  for (unsigned i = 0; i < 5; ++i) {
    qd.InitializeEmptyTable(tables[i], 8);
    BOOST_CHECK(qd.TableExists(tables[i]));
  }

  for (unsigned i = 0; i < 5; ++i) {
    qd.RemoveTable(tables[i]);
    for (unsigned j = 0; j <= i; ++j) BOOST_CHECK(!qd.TableExists(tables[j]));
    for (unsigned j = i + 1; j < 5; ++j) BOOST_CHECK(qd.TableExists(tables[j]));
  }
}

BOOST_AUTO_TEST_CASE(kind_operations) {
  QualityTablesFormatter qd("QualityTest.MS");

  qd.RemoveAllQualityTables();
  qd.InitializeEmptyTable(QualityTablesFormatter::KindNameTable, 4);
  BOOST_CHECK(qd.TableExists(QualityTablesFormatter::KindNameTable));

  unsigned kindIndex;
  BOOST_CHECK(
      !qd.QueryKindIndex(QualityTablesFormatter::MeanStatistic, kindIndex));

  unsigned originalKindIndex =
      qd.StoreKindName(QualityTablesFormatter::MeanStatistic);
  BOOST_CHECK(
      qd.QueryKindIndex(QualityTablesFormatter::MeanStatistic, kindIndex));
  BOOST_CHECK_EQUAL(kindIndex, originalKindIndex);
  BOOST_CHECK_EQUAL(qd.QueryKindIndex(QualityTablesFormatter::MeanStatistic),
                    originalKindIndex);

  unsigned secondKindIndex =
      qd.StoreKindName(QualityTablesFormatter::VarianceStatistic);
  BOOST_CHECK_NE(originalKindIndex, secondKindIndex);
  BOOST_CHECK_EQUAL(qd.QueryKindIndex(QualityTablesFormatter::MeanStatistic),
                    originalKindIndex);
  BOOST_CHECK_EQUAL(
      qd.QueryKindIndex(QualityTablesFormatter::VarianceStatistic),
      secondKindIndex);

  qd.InitializeEmptyTable(QualityTablesFormatter::KindNameTable, 4);
  BOOST_CHECK(
      !qd.QueryKindIndex(QualityTablesFormatter::MeanStatistic, kindIndex));
}

BOOST_AUTO_TEST_CASE(store_statistics) {
  QualityTablesFormatter qd("QualityTest.MS");

  qd.RemoveAllQualityTables();
  BOOST_CHECK(!qd.IsStatisticAvailable(QualityTablesFormatter::TimeDimension,
                                       QualityTablesFormatter::MeanStatistic));

  qd.InitializeEmptyTable(QualityTablesFormatter::KindNameTable, 4);
  BOOST_CHECK(!qd.IsStatisticAvailable(QualityTablesFormatter::TimeDimension,
                                       QualityTablesFormatter::MeanStatistic));

  qd.InitializeEmptyTable(QualityTablesFormatter::TimeStatisticTable, 4);
  BOOST_CHECK(!qd.IsStatisticAvailable(QualityTablesFormatter::TimeDimension,
                                       QualityTablesFormatter::MeanStatistic));

  unsigned meanStatIndex =
      qd.StoreKindName(QualityTablesFormatter::MeanStatistic);
  BOOST_CHECK(!qd.IsStatisticAvailable(QualityTablesFormatter::TimeDimension,
                                       QualityTablesFormatter::MeanStatistic));
  BOOST_CHECK_EQUAL(qd.QueryStatisticEntryCount(
                        QualityTablesFormatter::TimeDimension, meanStatIndex),
                    0u);

  StatisticalValue value(4);
  value.SetKindIndex(meanStatIndex);
  value.SetValue(0, std::complex<float>(0.0, 1.0));
  value.SetValue(1, std::complex<float>(2.0, -2.0));
  value.SetValue(2, std::complex<float>(-3.0, 3.0));
  value.SetValue(3, std::complex<float>(-4.0, -4.0));
  qd.StoreTimeValue(60.0, 107000000.0, value);
  BOOST_CHECK(qd.IsStatisticAvailable(QualityTablesFormatter::TimeDimension,
                                      QualityTablesFormatter::MeanStatistic));
  BOOST_CHECK_EQUAL(qd.QueryStatisticEntryCount(
                        QualityTablesFormatter::TimeDimension, meanStatIndex),
                    1u);

  std::vector<std::pair<QualityTablesFormatter::TimePosition, StatisticalValue>>
      entries;
  qd.QueryTimeStatistic(meanStatIndex, entries);
  BOOST_CHECK_EQUAL(entries.size(), (size_t)1);
  std::pair<QualityTablesFormatter::TimePosition, StatisticalValue> entry =
      entries[0];
  BOOST_CHECK_EQUAL(entry.first.frequency, 107000000.0f);
  BOOST_CHECK_EQUAL(entry.first.time, 60.0f);
  BOOST_CHECK_EQUAL(entry.second.PolarizationCount(), 4u);
  BOOST_CHECK_EQUAL(entry.second.KindIndex(), meanStatIndex);
  BOOST_CHECK_EQUAL(entry.second.Value(0), std::complex<float>(0.0, 1.0));
  BOOST_CHECK_EQUAL(entry.second.Value(1), std::complex<float>(2.0, -2.0));
  BOOST_CHECK_EQUAL(entry.second.Value(2), std::complex<float>(-3.0, 3.0));
  BOOST_CHECK_EQUAL(entry.second.Value(3), std::complex<float>(-4.0, -4.0));

  qd.RemoveTable(QualityTablesFormatter::KindNameTable);
  qd.RemoveTable(QualityTablesFormatter::TimeStatisticTable);
}

BOOST_AUTO_TEST_CASE(kind_names) {
  BOOST_CHECK_EQUAL(
      QualityTablesFormatter::KindToName(QualityTablesFormatter::MeanStatistic),
      "Mean");
  BOOST_CHECK_EQUAL(QualityTablesFormatter::KindToName(
                        QualityTablesFormatter::VarianceStatistic),
                    "Variance");
  BOOST_CHECK_EQUAL(
      QualityTablesFormatter::KindToName(QualityTablesFormatter::SumStatistic),
      "Sum");
  BOOST_CHECK_EQUAL(QualityTablesFormatter::KindToName(
                        QualityTablesFormatter::SumP2Statistic),
                    "SumP2");
  BOOST_CHECK_EQUAL(QualityTablesFormatter::KindToName(
                        QualityTablesFormatter::DMeanStatistic),
                    "DMean");
  BOOST_CHECK_EQUAL(QualityTablesFormatter::KindToName(
                        QualityTablesFormatter::DVarianceStatistic),
                    "DVariance");
  BOOST_CHECK_EQUAL(
      QualityTablesFormatter::KindToName(QualityTablesFormatter::DSumStatistic),
      "DSum");
  BOOST_CHECK_EQUAL(QualityTablesFormatter::KindToName(
                        QualityTablesFormatter::DSumP2Statistic),
                    "DSumP2");
  BOOST_CHECK_EQUAL(QualityTablesFormatter::KindToName(
                        QualityTablesFormatter::FTSumP2Statistic),
                    "FTSumP2");
}

BOOST_AUTO_TEST_SUITE_END()
