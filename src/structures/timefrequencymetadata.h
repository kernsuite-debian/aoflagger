#ifndef MSIO_TIME_FREQUENCY_META_DATA_H
#define MSIO_TIME_FREQUENCY_META_DATA_H

#include <string>
#include <vector>
#include <memory>

#include "antennainfo.h"

typedef std::shared_ptr<class TimeFrequencyMetaData> TimeFrequencyMetaDataPtr;
typedef std::shared_ptr<const class TimeFrequencyMetaData> TimeFrequencyMetaDataCPtr;

class TimeFrequencyMetaData
{
	public:
		TimeFrequencyMetaData()
			: _sequenceId(0), _valueDescription("Visibility"), _valueUnits("Jy")
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
			_valueDescription("Visibility"),
			_valueUnits("Jy")
		{
		}
		TimeFrequencyMetaData(const TimeFrequencyMetaData &source)
			: _sequenceId(source._sequenceId),
			_valueDescription(source._valueDescription),
			_valueUnits(source._valueUnits)
		{
			if(source._antenna1)
				_antenna1.reset(new AntennaInfo(*source._antenna1));
			if(source._antenna2)
				_antenna2.reset(new AntennaInfo(*source._antenna2));
			if(source._band)
				_band.reset(new BandInfo(*source._band));
			if(source._field)
				_field.reset(new FieldInfo(*source._field));
			if(source._observationTimes)
				_observationTimes.reset(new std::vector<double>(*source._observationTimes));
			if(source._uvw)
				_uvw.reset(new std::vector<class UVW>(*source._uvw));
		}
		
		TimeFrequencyMetaData& operator=(const TimeFrequencyMetaData &rhs)
		{
			_sequenceId = rhs._sequenceId;
			_valueDescription = rhs._valueDescription;
			_valueUnits = rhs._valueUnits;
			if(rhs._antenna1)
				_antenna1.reset(new AntennaInfo(*rhs._antenna1));
			if(rhs._antenna2)
				_antenna2.reset(new AntennaInfo(*rhs._antenna2));
			if(rhs._band)
				_band.reset(new BandInfo(*rhs._band));
			if(rhs._field)
				_field.reset(new FieldInfo(*rhs._field));
			if(rhs._observationTimes)
				_observationTimes.reset(new std::vector<double>(*rhs._observationTimes));
			if(rhs._uvw)
				_uvw.reset(new std::vector<class UVW>(*rhs._uvw));
			return *this;
		}

		const AntennaInfo &Antenna1() const { return *_antenna1; }
		void ClearAntenna1()
		{
			_antenna1.reset();
		}
		void SetAntenna1(const AntennaInfo &antenna1)
		{
			_antenna1.reset(new AntennaInfo(antenna1));
		}
		bool HasAntenna1() const { return static_cast<bool>(_antenna1); }

		const AntennaInfo &Antenna2() const { return *_antenna2; }
		void ClearAntenna2()
		{
			_antenna2.reset();
		}
		void SetAntenna2(const AntennaInfo &antenna2)
		{
			_antenna2.reset(new AntennaInfo(antenna2));
		}
		bool HasAntenna2() const { return static_cast<bool>(_antenna2); }

		const BandInfo &Band() const { return *_band; }
		void ClearBand()
		{
			_band.reset();
		}
		void SetBand(const BandInfo &band)
		{
			_band.reset(new BandInfo(band));
		}
		bool HasBand() const { return static_cast<bool>(_band); }

		const FieldInfo &Field() const { return *_field; }
		void ClearField()
		{
			_field.reset();
		}
		void SetField(const FieldInfo &field)
		{
			_field.reset(new FieldInfo(field));
		}
		bool HasField() const { return static_cast<bool>(_field); }

		const std::vector<double> &ObservationTimes() const {
			return *_observationTimes;
		}
		void ClearObservationTimes()
		{
			_observationTimes.reset();
		}
		void SetObservationTimes(const std::vector<double> &times)
		{
			_observationTimes.reset(new std::vector<double>(times));
		}
		bool HasObservationTimes() const { return static_cast<bool>(_observationTimes); }
		
		unsigned SequenceId() const { return _sequenceId; }
		void SetSequenceId(unsigned sequenceId) { _sequenceId = sequenceId; }
		
		const std::vector<class UVW> &UVW() const { return *_uvw; }
		void ClearUVW()
		{
			_uvw.reset();
		}
		void SetUVW(const std::vector<class UVW> &uvw)
		{
			_uvw.reset(new std::vector<class UVW>(uvw));
		}
		bool HasUVW() const { return static_cast<bool>(_uvw); }

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
		std::unique_ptr<AntennaInfo> _antenna1;
		std::unique_ptr<AntennaInfo> _antenna2;
		std::unique_ptr<BandInfo> _band;
		unsigned _sequenceId;
		std::unique_ptr<FieldInfo> _field;
		std::unique_ptr<std::vector<double>> _observationTimes;
		std::unique_ptr<std::vector<class UVW>> _uvw;
		std::string _valueDescription, _valueUnits;
};

#endif // MSIO_TIME_FREQUENCY_META_DATA_H
