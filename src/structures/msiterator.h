#ifndef MS_ITERATOR_H
#define MS_ITERATOR_H

#include "antennainfo.h"

#include <casacore/ms/MeasurementSets/MSColumns.h>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

class [[deprecated]] MSIterator {
public:
	MSIterator(const casacore::MeasurementSet& ms, bool hasCorrectedData=true);

	MSIterator &operator++() { _row++; return *this; }

	casacore::Complex Data(unsigned frequencyIndex, unsigned polarisation)
	{
		return (_dataCol)(_row)(casacore::IPosition(2, frequencyIndex, polarisation));
	}

	bool Flag(unsigned frequencyIndex, unsigned polarisation)
	{
		return (_flagCol)(_row)(casacore::IPosition(2, frequencyIndex, polarisation));
	}
	
	casacore::Array<bool>::const_iterator FlagIterator()
	{
		return (_flagCol)(_row).begin();
	}

	casacore::Complex CorrectedData(unsigned frequencyIndex, unsigned polarisation)
	{
		return (*_correctedDataCol)(_row)(casacore::IPosition(2, frequencyIndex, polarisation));
	}

	casacore::Array<casacore::Complex>::const_iterator CorrectedDataIterator()
	{
		return (*_correctedDataCol)(_row).begin();
	}

	int Field() { return (_fieldCol)(_row); }
	double Time() { return (_timeCol)(_row); }
	unsigned Antenna1() { return (_antenna1Col)(_row); }
	unsigned Antenna2() { return (_antenna2Col)(_row); }
	unsigned ScanNumber() { return (_scanNumberCol)(_row); }
	class UVW UVW() {
		class UVW uvw;
		casacore::Array<double> arr = (_uvwCol)(_row);
		casacore::Array<double>::const_iterator i = arr.begin();
		uvw.u = *i; ++i;
		uvw.v = *i; ++i;
		uvw.w = *i;
		return uvw;
	}
	unsigned Window() { return (_windowCol)(_row); }
	
private:
	MSIterator(const MSIterator&) = delete;
	MSIterator& operator=(const MSIterator&) = delete;
	unsigned long _row;
	
	casacore::MeasurementSet _ms;
	casacore::ScalarColumn<int> _antenna1Col;
	casacore::ScalarColumn<int> _antenna2Col;
	casacore::ArrayColumn<casacore::Complex> _dataCol;
	casacore::ArrayColumn<bool> _flagCol;
	std::unique_ptr<casacore::ArrayColumn<casacore::Complex>> _correctedDataCol;
	casacore::ScalarColumn<double> _timeCol;
	casacore::ScalarColumn<int> _fieldCol;
	casacore::ScalarColumn<int> _scanNumberCol;
	casacore::ArrayColumn<double> _uvwCol;
	casacore::ScalarColumn<int> _windowCol;
};

#endif
