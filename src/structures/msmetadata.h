#ifndef MS_META_DATA_H
#define MS_META_DATA_H

#include "../strategy/control/types.h"

#include "antennainfo.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <boost/optional/optional.hpp>

#include <string>
#include <vector>
#include <utility>
#include <set>

class MSMetaData
{
public:
	class Sequence;
	
	explicit MSMetaData(const std::string &path)
		: _path(path), _isMainTableDataInitialized(false)
	{
		initializeOtherData();
	}
	
	~MSMetaData();
	
	void SetIntervalStart(size_t index) { _intervalStart = index;}
	void SetIntervalEnd(size_t index) { _intervalEnd = index; }
	
	size_t FrequencyCount(size_t bandIndex)
	{
		return _bands[bandIndex].channels.size();
	}
	
	size_t TimestepCount()
	{
		initializeMainTableData();
		return _observationTimes.size();
	}
	
	size_t TimestepCount(size_t sequenceId)
	{
		initializeMainTableData();
		return _observationTimesPerSequence[sequenceId].size();
	}
	
	size_t PolarizationCount() const
	{
		return PolarizationCount(Path());
	}
	
	static size_t PolarizationCount(const std::string &filename);
	
	size_t AntennaCount() const
	{
		return _antennas.size();
	}
	
	size_t BandCount() const
	{ 
		return _bands.size();
	}
	
	size_t FieldCount() const
	{
		return _fields.size();
	}
	
	static size_t BandCount(const std::string &filename);
	
	/**
		* Get number of sequences. A sequence is a contiguous number of scans
		* on the same field. Thus, the next sequence starts as soon as the
		* fieldId changes (possibly to a previous field)
		*/
	size_t SequenceCount()
	{
		initializeMainTableData();
		return _observationTimesPerSequence.size();
	}
	
	const AntennaInfo &GetAntennaInfo(unsigned antennaId) const
	{
		return _antennas[antennaId];
	}
	
	//static BandInfo GetBandInfo(const std::string &filename, unsigned bandIndex);
	
	const BandInfo &GetBandInfo(unsigned bandIndex) const
	{
		return _bands[bandIndex];
	}
	
	const FieldInfo &GetFieldInfo(unsigned fieldIndex) const
	{
		return _fields[fieldIndex];
	}
	
	void GetDataDescToBandVector(std::vector<size_t>& dataDescToBand);
	
	std::string Path() const { return _path; }
	
	void GetBaselines(std::vector<std::pair<size_t,size_t> > &baselines)
	{
		initializeMainTableData();
		baselines = _baselines;
	}
	
	const std::vector<Sequence> &GetSequences()
	{
		initializeMainTableData();
		return _sequences;
	}
	
	const std::set<double> &GetObservationTimesSet()
	{
		initializeMainTableData();
		return _observationTimes;
	}
	
	const std::set<double> &GetObservationTimesSet(size_t sequenceId)
	{
		initializeMainTableData();
		return _observationTimesPerSequence[sequenceId];
	}
	
	bool HasAOFlaggerHistory();
	
	void GetAOFlaggerHistory(std::ostream &stream);
	
	void AddAOFlaggerHistory(const std::string& strategy, const std::string &commandline);
	
	std::string GetStationName() const;
	
	bool IsChannelZeroRubish();
	
	const std::string &TelescopeName() const { return _telescopeName; }
	
	class Sequence
	{
	public:
		Sequence(unsigned _antenna1, unsigned _antenna2, unsigned _spw, unsigned _sequenceId, unsigned _fieldId) :
			antenna1(_antenna1), antenna2(_antenna2),
			spw(_spw), sequenceId(_sequenceId),
			fieldId(_fieldId)
		{ }
		
		unsigned antenna1, antenna2;
		unsigned spw;
		unsigned sequenceId;
		unsigned fieldId;
		
		bool operator<(const Sequence &rhs) const
		{
			if(antenna1 < rhs.antenna1) return true;
			else if(antenna1 == rhs.antenna1)
			{
				if(antenna2 < rhs.antenna2) return true;
				else if(antenna2 == rhs.antenna2)
				{
					if(spw < rhs.spw) return true;
					else if(spw == rhs.spw)
					{
						return sequenceId < rhs.sequenceId;
					}
				}
			}
			return false;
		}
		
		bool operator==(const Sequence &rhs) const
		{
			return antenna1==rhs.antenna1 && antenna2==rhs.antenna2 &&
				spw==rhs.spw && sequenceId==rhs.sequenceId;
		}
	};

private:
	void initializeMainTableData();
	
	void initializeOtherData();
	
	void initializeAntennas(casacore::MeasurementSet& ms);
	void initializeBands(casacore::MeasurementSet& ms);
	void initializeFields(casacore::MeasurementSet& ms);
	void initializeObservation(casacore::MeasurementSet& ms);

	const std::string _path;
	
	bool _isMainTableDataInitialized;
	
	std::vector<std::pair<size_t,size_t> > _baselines;
	
	std::set<double> _observationTimes;
	
	std::vector<std::set<double> > _observationTimesPerSequence;

	std::vector<AntennaInfo> _antennas;
	
	std::vector<BandInfo> _bands;
	
	std::vector<FieldInfo> _fields;
	
	std::vector<Sequence> _sequences;
	
	std::string _telescopeName;
	
	boost::optional<size_t> _intervalStart, _intervalEnd;
};

#endif
