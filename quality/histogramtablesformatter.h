#ifndef HISTOGRAM_TABLES_FORMATTER_H
#define HISTOGRAM_TABLES_FORMATTER_H

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <cassert>
#include <memory>
#include <vector>

class HistogramTablesFormatter {
 public:
  enum TableKind { HistogramCountTable, HistogramTypeTable };

  struct HistogramItem {
    double binStart;
    double binEnd;
    double count;
  };

  enum HistogramType { TotalHistogram, RFIHistogram };

  explicit HistogramTablesFormatter(const std::string& measurementSetName)
      : _measurementSet(nullptr),
        _measurementSetName(measurementSetName),
        _typeTable(nullptr),
        _countTable(nullptr) {}

  ~HistogramTablesFormatter() { Close(); }

  void Close() {
    _countTable.reset();
    _typeTable.reset();
    closeMainTable();
  }

  std::string CountTableName() const { return "QUALITY_HISTOGRAM_COUNT"; }

  std::string TypeTableName() const { return "QUALITY_HISTOGRAM_TYPE"; }

  std::string TableName(enum TableKind table) const {
    switch (table) {
      case HistogramCountTable:
        return CountTableName();
      case HistogramTypeTable:
        return TypeTableName();
      default:
        return "";
    }
  }

  std::string TypeToName(HistogramType type) const {
    switch (type) {
      case TotalHistogram:
        return "Total";
      case RFIHistogram:
        return "RFI";
      default:
        return std::string();
    }
  }

  std::string TableFilename(enum TableKind table) const {
    return _measurementSetName + '/' + TableName(table);
  }

  bool TableExists(enum TableKind table) const {
    return _measurementSet->isReadable(TableFilename(table));
  }

  bool IsAvailable(unsigned typeIndex) {
    if (!TableExists(HistogramCountTable) || !TableExists(HistogramTypeTable))
      return false;
    return hasOneEntry(typeIndex);
  }

  void InitializeEmptyTables();

  void RemoveTable(enum TableKind table);

  void StoreValue(unsigned typeIndex, double binStart, double binEnd,
                  double count);

  void QueryHistogram(unsigned typeIndex,
                      std::vector<HistogramItem>& histogram);

  unsigned QueryTypeIndex(enum HistogramType type, unsigned polarizationIndex);
  bool QueryTypeIndex(enum HistogramType type, unsigned polarizationIndex,
                      unsigned& destTypeIndex);
  unsigned StoreOrQueryTypeIndex(enum HistogramType type,
                                 unsigned polarizationIndex) {
    unsigned typeIndex;
    if (QueryTypeIndex(type, polarizationIndex, typeIndex))
      return typeIndex;
    else
      return StoreType(type, polarizationIndex);
  }
  unsigned StoreType(enum HistogramType type, unsigned polarizationIndex);
  bool HistogramsExist() {
    return TableExists(HistogramCountTable) && TableExists(HistogramTypeTable);
  }
  void RemoveAll() {
    RemoveTable(HistogramCountTable);
    RemoveTable(HistogramTypeTable);
  }

 private:
  HistogramTablesFormatter(const HistogramTablesFormatter&) =
      delete;  // don't allow copies
  void operator=(const HistogramTablesFormatter&) =
      delete;  // don't allow assignment

  static const std::string ColumnNameType;
  static const std::string ColumnNameName;
  static const std::string ColumnNamePolarization;

  static const std::string ColumnNameBinStart;
  static const std::string ColumnNameBinEnd;
  static const std::string ColumnNameCount;

  std::unique_ptr<casacore::Table> _measurementSet;
  const std::string _measurementSetName;

  std::unique_ptr<casacore::Table> _typeTable;
  std::unique_ptr<casacore::Table> _countTable;

  bool hasOneEntry(unsigned typeIndex);
  void removeTypeEntry(enum HistogramType type, unsigned polarizationIndex);
  void removeEntries(enum TableKind table);

  void addTimeColumn(casacore::TableDesc& tableDesc);
  void addFrequencyColumn(casacore::TableDesc& tableDesc);
  void addValueColumn(casacore::TableDesc& tableDesc);

  void createTable(enum TableKind table) {
    switch (table) {
      case HistogramTypeTable:
        createTypeTable();
        break;
      case HistogramCountTable:
        createCountTable();
        break;
      default:
        break;
    }
  }

  void createTypeTable();
  void createCountTable();
  unsigned findFreeTypeIndex(casacore::Table& typeTable);

  void openMainTable(bool needWrite);
  void closeMainTable() { _measurementSet.reset(); }

  void openTable(TableKind table, bool needWrite,
                 std::unique_ptr<casacore::Table>& tablePtr);
  void openTypeTable(bool needWrite) {
    openTable(HistogramTypeTable, needWrite, _typeTable);
  }
  void openCountTable(bool needWrite) {
    openTable(HistogramCountTable, needWrite, _countTable);
  }
  casacore::Table& getTable(TableKind table, bool needWrite) {
    std::unique_ptr<casacore::Table>* tablePtr = nullptr;
    switch (table) {
      case HistogramTypeTable:
        tablePtr = &_typeTable;
        break;
      case HistogramCountTable:
        tablePtr = &_countTable;
        break;
    }
    assert(tablePtr);
    openTable(table, needWrite, *tablePtr);
    return **tablePtr;
  }
};

#endif
