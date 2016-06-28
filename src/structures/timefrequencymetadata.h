#ifndef MSIO_TIME_FREQUENCY_META_DATA_H
#define MSIO_TIME_FREQUENCY_META_DATA_H

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "antennainfo.h"

typedef boost::shared_ptr<class TimeFrequencyMetaData> TimeFrequencyMetaDataPtr;
typedef boost::shared_ptr<const class TimeFrequencyMetaData> TimeFrequencyMetaDataCPtr;

class TimeFrequencyMetaData
{
	public:
		TimeFrequencyMetaData()
			: _antenna1(0), _antenna2(0), _band(0), _sequenceId(0), _field(0), _observationTimes(0), _uvw(0), _valueDescription("Visibility"), _valueUnits("Jy")
		{
		}
		TimeFrequencyMetaData(const AntennaInfo &antenna1, const AntennaInfo &antenna2, const BandInfo &band, const FieldInfo &field, const std::vector<double> &observationTimes)
		:
			_antenna1(new AntennaInfo(antenna1)),
			_antenna2(new AntennaInfo(antenna2)),
			_band(new BandInfo(band)),
			_sequenceId(0),
			_field(new FieldInfo(field)),
			_observationTimes(new std::vector<double>(observationTimes)),
			_uvw(0),
			_valueDescription("Visibility"),
			_valueUnits("Jy")
		{
		}
		TimeFrequencyMetaData(const TimeFrequencyMetaData &source)
			: _antenna1(0), _antenna2(0), _band(0), _sequenceId(source._sequenceId), _field(0), _observationTimes(0), _uvw(0),
			_valueDescription(source._valueDescription),
			_valueUnits(source._valueUnits)
		{
			if(source._antenna1 != 0)
				_antenna1 = new AntennaInfo(*source._antenna1);
			if(source._antenna2 != 0)
				_antenna2 = new AntennaInfo(*source._antenna2);
			if(source._band != 0)
				_band = new BandInfo(*source._band);
			if(source._field != 0)
				_field = new FieldInfo(*source._field);
			if(source._observationTimes != 0)
				_observationTimes = new std::vector<double>(*source._observationTimes);
			if(source._uvw != 0)
				_uvw = new std::vector<class UVW>(*source._uvw);
		}
		~TimeFrequencyMetaData()
		{
			ClearAntenna1();
			ClearAntenna2();
			ClearBand();
			ClearField();
			ClearObservationTimes();
			ClearUVW();
		}

		const AntennaInfo &Antenna1() const { return *_antenna1; }
		void ClearAntenna1()
		{
			if(_antenna1 != 0)
			{
				delete _antenna1;
				_antenna1 = 0;
			}
		}
		void SetAntenna1(const AntennaInfo &antenna1)
		{
			ClearAntenna1();
			_antenna1 = new AntennaInfo(antenna1);
		}
		bool HasAntenna1() const { return _antenna1 != 0; }

		const AntennaInfo &Antenna2() const { return *_antenna2; }
		void ClearAntenna2()
		{
			if(_antenna2 != 0)
			{
				delete _antenna2;
				_antenna2 = 0;
			}
		}
		void SetAntenna2(const AntennaInfo &antenna2)
		{
			ClearAntenna2();
			_antenna2 = new AntennaInfo(antenna2);
		}
		bool HasAntenna2() const { return _antenna2 != 0; }

		const BandInfo &Band() const { return *_band; }
		void ClearBand()
		{
			if(_band != 0)
			{
				delete _band;
				_band = 0;
			}
		}
		void SetBand(const BandInfo &band)
		{
			ClearBand();
			_band = new BandInfo(band);
		}
		bool HasBand() const { return _band != 0; }

		const FieldInfo &Field() const { return *_field; }
		void ClearField()
		{
			if(_field != 0)
			{
				delete _field;
				_field = 0;
			}
		}
		void SetField(const FieldInfo &field)
		{
			ClearField();
			_field = new FieldInfo(field);
		}
		bool HasField() const { return _field != 0; }

		const std::vector<double> &ObservationTimes() const {
			return *_observationTimes;
		}
		void ClearObservationTimes()
		{
			if(_observationTimes != 0)
			{
				delete _observationTimes;
				_observationTimes = 0;
			}
		}
		void SetObservationTimes(const std::vector<double> &times)
		{
			ClearObservationTimes();
			_observationTimes = new std::vector<double>(times);
		}
		bool HasObservationTimes() const { return _observationTimes != 0; }
		
		unsigned SequenceId() const { return _sequenceId; }
		void SetSequenceId(unsigned sequenceId) { _sequenceId = sequenceId; }
		
		const std::vector<class UVW> &UVW() const { return *_uvw; }
		void ClearUVW()
		{
			if(_uvw != 0)
			{
				delete _uvw;
				_uvw = 0;
			}
		}
		void SetUVW(const std::vector<class UVW> &uvw)
		{
			ClearUVW();
			_uvw = new std::vector<class UVW>(uvw);
		}
		bool HasUVW() const { return _uvw != 0; }

		bool HasBaseline() const {
			return HasAntenna1() && HasAntenna2();
		}
		class Baseline Baseline() const {
			return ::Baseline(*_antenna1, *_antenna2);
		}
		
		const std::string &ValueDescription() const { return _valueDescription; }
		void SetValueDescription(const std::string &valueDescription)
		{
			_valueDescription = valueDescription;
		}
		
		const std::string &ValueUnits() const { return _valueUnits; }
		void SetValueUnits(const std::string &valueUnits)
		{
			_valueUnits = valueUnits;
		}
	private:
		void operator=(const TimeFrequencyMetaData &) { }
		
		AntennaInfo *_antenna1;
		AntennaInfo *_antenna2;
		BandInfo *_band;
		unsigned _sequenceId;
		FieldInfo *_field;
		std::vector<double> *_observationTimes;
		std::vector<class UVW> *_uvw;
		std::string _valueDescription, _valueUnits;
};

#endif // MSIO_TIME_FREQUENCY_META_DATA_H
