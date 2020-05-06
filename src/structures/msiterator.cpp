#include "msiterator.h"

MSIterator::MSIterator(const casacore::MeasurementSet& ms, bool hasCorrectedData) :
	_row(0),
	_ms(ms),
	_antenna1Col(_ms, "ANTENNA1"),
	_antenna2Col(_ms, "ANTENNA2"),
	_dataCol(_ms, "DATA"),
	_flagCol(_ms, "FLAG"),
	_correctedDataCol(hasCorrectedData ? new casacore::ArrayColumn<casacore::Complex>(_ms, "CORRECTED_DATA") : nullptr),
	_timeCol(_ms, "TIME"),
	_fieldCol(_ms, "FIELD_ID"),
	_scanNumberCol(_ms, "SCAN_NUMBER"),
	_uvwCol(_ms, "UVW"),
	_windowCol(_ms, "DATA_DESC_ID")
{ }
