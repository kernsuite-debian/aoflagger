#ifndef MSIO_QUALITY_DATA_H
#define MSIO_QUALITY_DATA_H

#include <memory>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#define QUALITY_TABLES_VERSION 1
#define QUALITY_TABLES_VERSION_STR "1"

class StatisticalValue;

class QualityTablesFormatter {
 public:
  enum StatisticKind {
    CountStatistic,
    SumStatistic,
    MeanStatistic,
    RFICountStatistic,
    RFISumStatistic,
    RFIMeanStatistic,
    RFIRatioStatistic,
    RFIPercentageStatistic,
    FlaggedCountStatistic,
    FlaggedRatioStatistic,
    SumP2Statistic,
    SumP3Statistic,
    SumP4Statistic,
    VarianceStatistic,
    VarianceOfVarianceStatistic,
    StandardDeviationStatistic,
    SkewnessStatistic,
    KurtosisStatistic,
    SignalToNoiseStatistic,
    DSumStatistic,
    DMeanStatistic,
    DSumP2Statistic,
    DSumP3Statistic,
    DSumP4Statistic,
    DVarianceStatistic,
    DVarianceOfVarianceStatistic,
    DStandardDeviationStatistic,
    DCountStatistic,
    BadSolutionCountStatistic,
    CorrectCountStatistic,
    CorrectedMeanStatistic,
    CorrectedSumP2Statistic,
    CorrectedDCountStatistic,
    CorrectedDMeanStatistic,
    CorrectedDSumP2Statistic,
    FTSumStatistic,
    FTSumP2Statistic,
    EndPlaceHolderStatistic
  };

  enum StatisticDimension {
    TimeDimension,
    FrequencyDimension,
    BaselineDimension,
    BaselineTimeDimension
  };

  enum QualityTable {
    KindNameTable,
    TimeStatisticTable,
    FrequencyStatisticTable,
    BaselineStatisticTable,
    BaselineTimeStatisticTable
  };

  struct TimePosition {
    double time;
    double frequency;
  };

  struct FrequencyPosition {
    double frequency;
  };

  struct BaselinePosition {
    unsigned antenna1;
    unsigned antenna2;
    double frequency;
  };

  struct BaselineTimePosition {
    double time;
    unsigned antenna1;
    unsigned antenna2;
    double frequency;
  };

  explicit QualityTablesFormatter(const std::string& measurementSetName)
      : _measurementSet(),
        _measurementSetName(measurementSetName),
        _kindNameTable(),
        _timeTable(),
        _frequencyTable(),
        _baselineTable(),
        _baselineTimeTable() {}

  ~QualityTablesFormatter() { Close(); }

  void Close() {
    _kindNameTable.reset();
    _timeTable.reset();
    _frequencyTable.reset();
    _baselineTable.reset();
    _baselineTimeTable.reset();

    _measurementSet.reset();
  }

  bool TableExists(enum QualityTable table) const {
    return _measurementSet->isReadable(TableToFilename(table));
  }

  static const std::string& KindToName(const enum StatisticKind kind) {
    return _kindToNameTable[(int)kind];
  }

  static enum StatisticKind NameToKind(const std::string& kindName);

  static const std::string& TableToName(const enum QualityTable table) {
    return _tableToNameTable[(int)table];
  }

  const std::string TableToFilename(const enum QualityTable table) const {
    return _measurementSetName + '/' + TableToName(table);
  }

  enum QualityTable DimensionToTable(
      const enum StatisticDimension dimension) const {
    return _dimensionToTableTable[(int)dimension];
  }

  bool IsStatisticAvailable(enum StatisticDimension dimension,
                            enum StatisticKind kind);

  void InitializeEmptyStatistic(enum StatisticDimension dimension,
                                enum StatisticKind kind,
                                unsigned polarizationCount);

  void InitializeEmptyTable(enum QualityTable table,
                            unsigned polarizationCount);

  void RemoveTable(enum QualityTable table);

  void RemoveAllQualityTables() {
    RemoveTable(BaselineTimeStatisticTable);
    RemoveTable(BaselineStatisticTable);
    RemoveTable(FrequencyStatisticTable);
    RemoveTable(TimeStatisticTable);
    RemoveTable(KindNameTable);
  }

  unsigned StoreKindName(enum StatisticKind kind) {
    return StoreKindName(KindToName(kind));
  }

  unsigned StoreKindName(const std::string& name);

  void StoreTimeValue(double time, double frequency,
                      const StatisticalValue& value);
  void StoreFrequencyValue(double frequency, const StatisticalValue& value);
  void StoreBaselineValue(unsigned antenna1, unsigned antenna2,
                          double frequency, const StatisticalValue& value);
  void StoreBaselineTimeValue(unsigned antenna1, unsigned antenna2, double time,
                              double frequency, const StatisticalValue& value);

  unsigned QueryKindIndex(enum StatisticKind kind);
  bool QueryKindIndex(enum StatisticKind kind, unsigned& destKindIndex);
  unsigned StoreOrQueryKindIndex(enum StatisticKind kind) {
    unsigned kindIndex;
    if (QueryKindIndex(kind, kindIndex))
      return kindIndex;
    else
      return StoreKindName(kind);
  }

  unsigned QueryStatisticEntryCount(enum StatisticDimension dimension,
                                    unsigned kindIndex);

  void QueryTimeStatistic(
      unsigned kindIndex,
      std::vector<std::pair<TimePosition, StatisticalValue>>& entries);
  void QueryFrequencyStatistic(
      unsigned kindIndex,
      std::vector<std::pair<FrequencyPosition, StatisticalValue>>& entries);
  void QueryBaselineStatistic(
      unsigned kindIndex,
      std::vector<std::pair<BaselinePosition, StatisticalValue>>& entries);
  void QueryBaselineTimeStatistic(
      unsigned kindIndex,
      std::vector<std::pair<BaselineTimePosition, StatisticalValue>>& entries);

  unsigned GetPolarizationCount();

 private:
  QualityTablesFormatter(const QualityTablesFormatter&) =
      delete;  // don't allow copies
  void operator=(const QualityTablesFormatter&) =
      delete;  // don't allow assignment

  static const std::string _kindToNameTable[];
  static const std::string _tableToNameTable[];
  static const QualityTable _dimensionToTableTable[];

  static const std::string ColumnNameAntenna1;
  static const std::string ColumnNameAntenna2;
  static const std::string ColumnNameFrequency;
  static const std::string ColumnNameKind;
  static const std::string ColumnNameName;
  static const std::string ColumnNameTime;
  static const std::string ColumnNameValue;

  std::unique_ptr<casacore::Table> _measurementSet;
  const std::string _measurementSetName;

  std::unique_ptr<casacore::Table> _kindNameTable;
  std::unique_ptr<casacore::Table> _timeTable;
  std::unique_ptr<casacore::Table> _frequencyTable;
  std::unique_ptr<casacore::Table> _baselineTable;
  std::unique_ptr<casacore::Table> _baselineTimeTable;

  bool hasOneEntry(enum QualityTable table, unsigned kindIndex);
  void removeStatisticFromStatTable(enum QualityTable table,
                                    enum StatisticKind kind);
  void removeKindNameEntry(enum StatisticKind kind);
  void removeEntries(enum QualityTable table);

  /**
   * Add the time column to the table descriptor. Used by create..Table()
   * methods. It holds "Measure"s of time, which is what casacore defines as a
   * value including a unit and a reference frame.
   */
  void addTimeColumn(casacore::TableDesc& tableDesc);

  /**
   * Add the frequency column to the table descriptor. Used by create..Table()
   * methods. It holds "Quantum"s of frequency, which is what casacore defines
   * as a value including a unit (Hertz).
   */
  void addFrequencyColumn(casacore::TableDesc& tableDesc);

  /**
   * Add value column to the table descriptor. Used by create..Table() methods.
   * It consist of an array of statistics, each element holds a polarization.
   */
  void addValueColumn(casacore::TableDesc& tableDesc,
                      unsigned polarizationCount);

  void createTable(enum QualityTable table, unsigned polarizationCount) {
    switch (table) {
      case KindNameTable:
        createKindNameTable();
        break;
      case TimeStatisticTable:
        createTimeStatisticTable(polarizationCount);
        break;
      case FrequencyStatisticTable:
        createFrequencyStatisticTable(polarizationCount);
        break;
      case BaselineStatisticTable:
        createBaselineStatisticTable(polarizationCount);
        break;
      case BaselineTimeStatisticTable:
        createBaselineTimeStatisticTable(polarizationCount);
        break;
      default:
        break;
    }
  }

  /**
   * Will add an empty table to the measurement set named "QUALITY_KIND_NAME"
   * and initialize its default column. This table can hold a list of quality
   * statistic types that are referred to in the statistic value tables.
   */
  void createKindNameTable();
  /**
   * Will add an empty table to the measurement set named
   * "QUALITY_TIME_STATISTIC" and initialize its default column. This table can
   * hold several statistic kinds per time step.
   * @param polarizationCount specifies the nr polarizations. This is required
   * for the shape of the value column.
   */
  void createTimeStatisticTable(unsigned polarizationCount);
  /**
   * Will add an empty table to the measurement set named
   * "QUALITY_FREQUENCY_STATISTIC" and initialize its default column. This table
   * can hold several statistic kinds per time step.
   * @param polarizationCount specifies the nr polarizations. This is required
   * for the shape of the value column.
   */
  void createFrequencyStatisticTable(unsigned polarizationCount);
  /**
   * Will add an empty table to the measurement set named
   * "QUALITY_BASELINE_STATISTIC" and initialize its default column. This table
   * can hold several statistic kinds per time step.
   * @param polarizationCount specifies the nr polarizations. This is required
   * for the shape of the value column.
   */
  void createBaselineStatisticTable(unsigned polarizationCount);
  void createBaselineTimeStatisticTable(unsigned polarizationCount);
  unsigned findFreeKindIndex(casacore::Table& kindTable);

  void openMainTable(bool needWrite);

  void openTable(QualityTable table, bool needWrite,
                 std::unique_ptr<casacore::Table>& tablePtr);
  void openKindNameTable(bool needWrite) {
    openTable(KindNameTable, needWrite, _kindNameTable);
  }
  void openTimeTable(bool needWrite) {
    openTable(TimeStatisticTable, needWrite, _timeTable);
  }
  void openFrequencyTable(bool needWrite) {
    openTable(FrequencyStatisticTable, needWrite, _frequencyTable);
  }
  void openBaselineTable(bool needWrite) {
    openTable(BaselineStatisticTable, needWrite, _baselineTable);
  }
  void openBaselineTimeTable(bool needWrite) {
    openTable(BaselineTimeStatisticTable, needWrite, _baselineTimeTable);
  }
  casacore::Table& getTable(QualityTable table, bool needWrite) {
    std::unique_ptr<casacore::Table>* tablePtr = nullptr;
    switch (table) {
      case KindNameTable:
        tablePtr = &_kindNameTable;
        break;
      case TimeStatisticTable:
        tablePtr = &_timeTable;
        break;
      case FrequencyStatisticTable:
        tablePtr = &_frequencyTable;
        break;
      case BaselineStatisticTable:
        tablePtr = &_baselineTable;
        break;
      case BaselineTimeStatisticTable:
        tablePtr = &_baselineTimeTable;
        break;
    }
    openTable(table, needWrite, *tablePtr);
    return **tablePtr;
  }
};

#endif
