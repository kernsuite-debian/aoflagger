#ifndef MS_SELECTION_H
#define MS_SELECTION_H

#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <limits>
#include <map>
#include <vector>

class MSSelection
{
public:
	MSSelection(casacore::MeasurementSet& ms,
		const std::vector<std::map<double, size_t>>& observationTimes
	) :
		_observationTimes(observationTimes),
		_ms(ms)
	{ 
	}
	
	template<typename Function>
	void Process(Function function)
	{
		casacore::ScalarColumn<double> timeColumn(_ms, "TIME");
		casacore::ScalarColumn<int> fieldIdColumn(_ms, "FIELD_ID");
		
		double prevTime = -1.0;
		size_t
			prevFieldId = size_t(-1),
			sequenceId = size_t(-1),
			timeIndexInSequence = size_t(-1);
	
		for(size_t rowIndex = 0; rowIndex != _ms.nrow(); ++rowIndex)
		{
			double time = timeColumn(rowIndex);
			bool newTime = time != prevTime;
			size_t fieldId = fieldIdColumn(rowIndex);
			if(fieldId != prevFieldId)
			{
				prevFieldId = fieldId;
				sequenceId++;
				newTime = true;
			}
			if(newTime)
			{
				const std::map<double, size_t>
					&observationTimes = _observationTimes[sequenceId];
				prevTime = time;
				auto elem = observationTimes.find(time);
				if(elem == observationTimes.end())
					timeIndexInSequence = std::numeric_limits<size_t>::max();
				else
					timeIndexInSequence = elem->second;
			}
			if(timeIndexInSequence != std::numeric_limits<size_t>::max())
			{
				function(rowIndex, sequenceId, timeIndexInSequence);
			}
		}
	}
	
private:
	std::vector<std::map<double, size_t>> _observationTimes;
	
	casacore::MeasurementSet& _ms;
};

#endif
