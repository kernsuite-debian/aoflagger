#ifndef MS_ROW_DATA_H
#define MS_ROW_DATA_H

#include <string.h>

#include <casacore/ms/MeasurementSets/MSColumns.h>

#include "types.h"

#include "../util/serializable.h"

/**
 * Encapsulates the information that is contained in the Data column in one row
 * of a measurement set.
 * If some of the meta data needs to be stored as well, use the MSRowDataExt class.
 * @see MSRowDataExt
 */
class MSRowData : public Serializable
{
	public:
		MSRowData() = default;
    
		MSRowData(unsigned polarizationCount, unsigned channelCount) :
		_polarizationCount(polarizationCount),
		_channelCount(channelCount),
		_realData(polarizationCount * channelCount),
		_imagData(polarizationCount * channelCount)
		{ }
		
		virtual void Serialize(std::ostream &stream) const final override
		{
			SerializeToUInt32(stream, _polarizationCount);
			SerializeToUInt32(stream, _channelCount);
			for(num_t val : _realData)
				SerializeToFloat(stream, val);
			for(num_t val : _imagData)
				SerializeToFloat(stream, val);
		}
		
		virtual void Unserialize(std::istream &stream) final override
		{
			_polarizationCount = UnserializeUInt32(stream);
			_channelCount = UnserializeUInt32(stream);
			const size_t size = _polarizationCount * _channelCount;
      _realData.resize(size);
      _imagData.resize(size);
			for(size_t i=0 ; i<size; ++i)
				_realData[i] = UnserializeFloat(stream);
			for(size_t i=0 ; i<size; ++i)
				_imagData[i] = UnserializeFloat(stream);
		}
		unsigned PolarizationCount() const { return _polarizationCount; }
		unsigned ChannelCount() const { return _channelCount; }
		const num_t *RealPtr() const { return _realData.data(); }
		const num_t *ImagPtr() const { return _imagData.data(); }
		num_t *RealPtr() { return _realData.data(); }
		num_t *ImagPtr() { return _imagData.data(); }
		const num_t *RealPtr(size_t channel) const { return &_realData[_polarizationCount * channel]; }
		const num_t *ImagPtr(size_t channel) const { return &_imagData[_polarizationCount * channel]; }
		num_t *RealPtr(size_t channel) { return &_realData[_polarizationCount * channel]; }
		num_t *ImagPtr(size_t channel) { return &_imagData[_polarizationCount * channel]; }
	private:
		unsigned _polarizationCount, _channelCount;
    std::vector<num_t> _realData, _imagData;
};

#endif
