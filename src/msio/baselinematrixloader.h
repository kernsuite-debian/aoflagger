#ifndef BASELINEMATRIXLOADER_H
#define BASELINEMATRIXLOADER_H

#include <cstring>

#include <casacore/tables/Tables/TableIter.h>

#include "../structures/timefrequencydata.h"
#include "../structures/msmetadata.h"

/**
 * Loader for antenna x antenna matrices, useful for e.g. spatial analyses such as spatial filtering.
 */
class BaselineMatrixLoader
{
public:
	explicit BaselineMatrixLoader(MSMetaData &measurementSet);

	TimeFrequencyData Load(size_t timeIndex)
	{
		return LoadSummed(timeIndex);
	}
	void LoadPerChannel(size_t timeIndex, std::vector<TimeFrequencyData> &data);

	size_t TimeIndexCount() const { return _timeIndexCount; }
	class SpatialMatrixMetaData &MetaData() const
	{
		return *_metaData;
	}
	size_t FrequencyCount() const { return _frequencyCount; }
	
private:
	TimeFrequencyData LoadSummed(size_t timeIndex);

	std::unique_ptr<casacore::Table> _sortedTable;
	std::unique_ptr<casacore::TableIterator> _tableIter;
	size_t _currentIterIndex;
	MSMetaData _msMetaData;
	size_t _timeIndexCount;
	std::unique_ptr<class SpatialMatrixMetaData> _metaData;
	size_t _frequencyCount;
};

#endif
