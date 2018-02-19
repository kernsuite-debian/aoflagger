#ifndef MS_ROW_DATAEXT_H
#define MS_ROW_DATAEXT_H

#include "msrowdata.h"

#include "../util/serializable.h"

class MSRowDataExt : public Serializable
{
	public:
		MSRowDataExt() = default;
		
		MSRowDataExt(unsigned polarizationCount, unsigned channelCount) :
			_data(polarizationCount, channelCount),
			_antenna1(),
			_antenna2(),
			_timeOffsetIndex(),
			_u(), _v(), _w(),
			_time()
		{
		}
		
		const MSRowData &Data() const { return _data; }
		MSRowData &Data() { return _data; }
		unsigned Antenna1() const { return _antenna1; }
		unsigned Antenna2() const { return _antenna2; }
		double U() const { return _u; }
		double V() const { return _v; }
		double W() const { return _w; }
		double Time() const { return _time; }
		size_t TimeOffsetIndex() const { return _timeOffsetIndex; }
		
		void SetU(double u) { _u = u; }
		void SetV(double v) { _v = v; }
		void SetW(double w) { _w = w; }
		void SetAntenna1(double antenna1) { _antenna1 = antenna1; }
		void SetAntenna2(double antenna2) { _antenna2 = antenna2; }
		void SetTime(double time) { _time = time; }
		void SetTimeOffsetIndex(size_t timeOffsetIndex) { _timeOffsetIndex = timeOffsetIndex; }
		
		virtual void Serialize(std::ostream &stream) const final override
		{
			_data.Serialize(stream);
			SerializeToUInt32(stream, _antenna1);
			SerializeToUInt32(stream, _antenna2);
			SerializeToUInt64(stream, _timeOffsetIndex);
			SerializeToDouble(stream, _u);
			SerializeToDouble(stream, _v);
			SerializeToDouble(stream, _w);
			SerializeToDouble(stream, _time);
		}
		
		virtual void Unserialize(std::istream &stream) final override
		{
			_data.Unserialize(stream);
			_antenna1 = UnserializeUInt32(stream);
			_antenna2 = UnserializeUInt32(stream);
			_timeOffsetIndex = UnserializeUInt64(stream);
			_u = UnserializeDouble(stream);
			_v = UnserializeDouble(stream);
			_w = UnserializeDouble(stream);
			_time = UnserializeDouble(stream);
		}
	private:
		MSRowData _data;
		unsigned _antenna1, _antenna2;
		size_t _timeOffsetIndex;
		double _u, _v, _w;
		double _time;
};

#endif
