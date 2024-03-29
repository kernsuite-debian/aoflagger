#include "histogramtablesformatter.h"

#include <stdexcept>

#include <casacore/ms/MeasurementSets/MSColumns.h>

#include <casacore/tables/Tables/ScaColDesc.h>
#include <casacore/tables/Tables/SetupNewTab.h>
#include <casacore/tables/Tables/TableRecord.h>

#include <casacore/measures/TableMeasures/TableMeasDesc.h>

#include <casacore/measures/Measures/MEpoch.h>

#include "statisticalvalue.h"

const std::string HistogramTablesFormatter::ColumnNameType = "TYPE";
const std::string HistogramTablesFormatter::ColumnNameName = "NAME";
const std::string HistogramTablesFormatter::ColumnNameCount = "COUNT";
const std::string HistogramTablesFormatter::ColumnNamePolarization =
    "POLARIZATION";
const std::string HistogramTablesFormatter::ColumnNameBinStart = "BIN_START";
const std::string HistogramTablesFormatter::ColumnNameBinEnd = "BIN_END";

void HistogramTablesFormatter::InitializeEmptyTables() {
  if (TableExists(HistogramCountTable))
    removeEntries(HistogramCountTable);
  else
    createCountTable();

  if (TableExists(HistogramTypeTable))
    removeEntries(HistogramTypeTable);
  else
    createTypeTable();
}

void HistogramTablesFormatter::RemoveTable(enum TableKind table) {
  if (TableExists(table)) {
    Close();
    openMainTable(true);
    if (_measurementSet->keywordSet().isDefined(TableName(table)))
      _measurementSet->rwKeywordSet().removeField(TableName(table));
    if (_measurementSet->isReadable(TableFilename(table)))
      casacore::Table::deleteTable(TableFilename(table));
  }
}

unsigned HistogramTablesFormatter::QueryTypeIndex(enum HistogramType type,
                                                  unsigned polarizationIndex) {
  unsigned kindIndex;
  if (!QueryTypeIndex(type, polarizationIndex, kindIndex))
    throw std::runtime_error(
        "getKindIndex(): Requested statistic kind not available.");
  return kindIndex;
}

bool HistogramTablesFormatter::QueryTypeIndex(enum HistogramType type,
                                              unsigned polarizationIndex,
                                              unsigned& destTypeIndex) {
  openTypeTable(false);
  const casacore::ROScalarColumn<int> typeColumn(*_typeTable, ColumnNameType);
  const casacore::ROScalarColumn<int> polarizationColumn(
      *_typeTable, ColumnNamePolarization);
  const casacore::ROScalarColumn<casacore::String> nameColumn(*_typeTable,
                                                              ColumnNameName);
  const casacore::String nameToFind(TypeToName(type));

  const unsigned nrRow = _typeTable->nrow();

  for (unsigned i = 0; i < nrRow; ++i) {
    if ((unsigned)polarizationColumn(i) == polarizationIndex &&
        nameColumn(i) == nameToFind) {
      destTypeIndex = typeColumn(i);
      return true;
    }
  }
  return false;
}

bool HistogramTablesFormatter::hasOneEntry(unsigned typeIndex) {
  const casacore::Table& casaTable = getTable(HistogramCountTable, false);
  const casacore::ROScalarColumn<int> typeColumn(casaTable, ColumnNameType);

  const unsigned nrRow = casaTable.nrow();

  for (unsigned i = 0; i < nrRow; ++i) {
    if (typeColumn(i) == (int)typeIndex) return true;
  }
  return false;
}

void HistogramTablesFormatter::createTypeTable() {
  casacore::TableDesc tableDesc(TypeTableName() + "_TYPE", "1.0",
                                casacore::TableDesc::Scratch);
  tableDesc.comment() =
      "Couples the TYPE column in the QUALITY_HISTOGRAM_COUNT table to the "
      "name and polarization of the histogram";
  tableDesc.addColumn(casacore::ScalarColumnDesc<int>(
      ColumnNameType, "Index of the statistic kind"));
  tableDesc.addColumn(casacore::ScalarColumnDesc<int>(
      ColumnNamePolarization,
      "Index of the polarization corresponding to the main table"));
  tableDesc.addColumn(casacore::ScalarColumnDesc<casacore::String>(
      ColumnNameName, "Name of the statistic"));

  casacore::SetupNewTable newTableSetup(TableFilename(HistogramTypeTable),
                                        tableDesc, casacore::Table::New);
  const casacore::Table newTable(newTableSetup);
  openMainTable(true);
  _measurementSet->rwKeywordSet().defineTable(TypeTableName(), newTable);
}

void HistogramTablesFormatter::createCountTable() {
  casacore::TableDesc tableDesc(CountTableName() + "_TYPE", "1.0",
                                casacore::TableDesc::Scratch);
  tableDesc.comment() = "Histograms of the data in the main table";
  tableDesc.addColumn(casacore::ScalarColumnDesc<int>(
      ColumnNameType, "Index of the statistic kind"));
  tableDesc.addColumn(casacore::ScalarColumnDesc<double>(
      ColumnNameBinStart,
      "Lower start value of the bin corresponding to the count"));
  tableDesc.addColumn(casacore::ScalarColumnDesc<double>(
      ColumnNameBinEnd,
      "Higher end value of the bin corresponding to the count"));
  tableDesc.addColumn(
      casacore::ScalarColumnDesc<double>(ColumnNameCount, "Histogram y-value"));

  casacore::SetupNewTable newTableSetup(TableFilename(HistogramCountTable),
                                        tableDesc, casacore::Table::New);
  const casacore::Table newTable(newTableSetup);
  openMainTable(true);
  _measurementSet->rwKeywordSet().defineTable(CountTableName(), newTable);
}

unsigned HistogramTablesFormatter::StoreType(enum HistogramType type,
                                             unsigned polarizationIndex) {
  openTypeTable(true);

  const unsigned typeIndex = findFreeTypeIndex(*_typeTable);

  const unsigned newRow = _typeTable->nrow();
  _typeTable->addRow();
  casacore::ScalarColumn<int> typeColumn(*_typeTable, ColumnNameType);
  casacore::ScalarColumn<int> polarizationColumn(*_typeTable,
                                                 ColumnNamePolarization);
  casacore::ScalarColumn<casacore::String> nameColumn(*_typeTable,
                                                      ColumnNameName);
  typeColumn.put(newRow, typeIndex);
  polarizationColumn.put(newRow, polarizationIndex);
  nameColumn.put(newRow, TypeToName(type));
  return typeIndex;
}

unsigned HistogramTablesFormatter::findFreeTypeIndex(
    casacore::Table& typeTable) {
  int maxIndex = 0;

  const casacore::ROScalarColumn<int> typeColumn(typeTable, ColumnNameType);

  const unsigned nrRow = typeTable.nrow();

  for (unsigned i = 0; i < nrRow; ++i) {
    if (typeColumn(i) > maxIndex) maxIndex = typeColumn(i);
  }
  return maxIndex + 1;
}

void HistogramTablesFormatter::openTable(
    enum TableKind table, bool needWrite,
    std::unique_ptr<casacore::Table>& tablePtr) {
  if (!tablePtr) {
    openMainTable(false);
    tablePtr = std::make_unique<casacore::Table>(
        _measurementSet->keywordSet().asTable(TableName(table)));
    if (needWrite) tablePtr->reopenRW();
  } else {
    if (needWrite && !tablePtr->isWritable()) tablePtr->reopenRW();
  }
}

void HistogramTablesFormatter::StoreValue(unsigned typeIndex, double binStart,
                                          double binEnd, double count) {
  openCountTable(true);

  const unsigned newRow = _countTable->nrow();
  _countTable->addRow();

  casacore::ScalarColumn<int> typeColumn(*_countTable, ColumnNameType);
  casacore::ScalarColumn<double> binStartColumn(*_countTable,
                                                ColumnNameBinStart);
  casacore::ScalarColumn<double> binEndColumn(*_countTable, ColumnNameBinEnd);
  casacore::ScalarColumn<double> countColumn(*_countTable, ColumnNameCount);

  typeColumn.put(newRow, typeIndex);
  binStartColumn.put(newRow, binStart);
  binEndColumn.put(newRow, binEnd);
  countColumn.put(newRow, count);
}

void HistogramTablesFormatter::removeTypeEntry(enum HistogramType type,
                                               unsigned polarizationIndex) {
  openTypeTable(true);
  const casacore::ScalarColumn<int> polarizationColumn(*_typeTable,
                                                       ColumnNamePolarization);
  const casacore::ScalarColumn<casacore::String> nameColumn(*_typeTable,
                                                            ColumnNameName);

  const unsigned nrRow = _typeTable->nrow();
  const casacore::String typeName(TypeToName(type));

  for (unsigned i = 0; i < nrRow; ++i) {
    if (nameColumn(i) == typeName &&
        (unsigned)polarizationColumn(i) == polarizationIndex) {
      _typeTable->removeRow(i);
      break;
    }
  }
}

void HistogramTablesFormatter::removeEntries(enum TableKind table) {
  casacore::Table& casaTable = getTable(table, true);
  const unsigned nrRow = casaTable.nrow();
  for (int i = nrRow - 1; i >= 0; --i) {
    casaTable.removeRow(i);
  }
}

void HistogramTablesFormatter::QueryHistogram(
    unsigned typeIndex, std::vector<HistogramItem>& histogram) {
  const casacore::Table& table(getTable(HistogramCountTable, false));
  const unsigned nrRow = table.nrow();

  const casacore::ROScalarColumn<int> typeColumn(table, ColumnNameType);
  const casacore::ROScalarColumn<double> binStartColumn(table,
                                                        ColumnNameBinStart);
  const casacore::ROScalarColumn<double> binEndColumn(table, ColumnNameBinEnd);
  const casacore::ROScalarColumn<double> countColumn(table, ColumnNameCount);

  for (unsigned i = 0; i < nrRow; ++i) {
    if (typeColumn(i) == (int)typeIndex) {
      HistogramItem item;
      item.binStart = binStartColumn(i);
      item.binEnd = binEndColumn(i);
      item.count = countColumn(i);
      histogram.push_back(item);
    }
  }
}

void HistogramTablesFormatter::openMainTable(bool needWrite) {
  if (_measurementSet == nullptr) {
    if (needWrite)
      _measurementSet = std::make_unique<casacore::Table>(
          _measurementSetName, casacore::Table::Update);
    else
      _measurementSet = std::make_unique<casacore::Table>(_measurementSetName);
  } else if (needWrite) {
    if (!_measurementSet->isWritable()) _measurementSet->reopenRW();
  }
}
