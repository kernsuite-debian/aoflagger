#ifndef PYTHON_DATA_H
#define PYTHON_DATA_H

#include "../structures/timefrequencydata.h"

#include <boost/python/list.hpp>

namespace aoflagger_python
{
	class Data
	{
	public:
		Data() = default;
		
		Data(const TimeFrequencyData& tfData) : _tfData(tfData)
		{ }
		
		Data(TimeFrequencyData&& tfData) : _tfData(std::move(tfData))
		{ }
		
		Data operator-(const Data& other) const
		{
			return Data(TimeFrequencyData::MakeFromDiff(_tfData, other.TFData()));
		}
		
		void clear_mask()
		{
			_tfData.SetNoMask();
		}
		
		Data convert_to_polarization(PolarizationEnum polarization) const
		{
			return Data(_tfData.Make(polarization));
		}
		
		Data convert_to_complex(enum TimeFrequencyData::ComplexRepresentation complexRepresentation) const
		{
			return Data(_tfData.Make(complexRepresentation));
		}
		
		Data copy() const
		{
			return Data(_tfData);
		}
		
		void join_mask(const Data& other)
		{
			_tfData.JoinMask(other._tfData);
		}
		
		Data make_complex() const
		{
			return Data(_tfData.MakeFromComplexCombination(_tfData, _tfData));
		}
		
		boost::python::list polarizations() const
		{
			const std::vector<PolarizationEnum> pols = _tfData.Polarizations();
			boost::python::list polList;
			for (PolarizationEnum p : pols)
				polList.append(p);
			return polList;
		}
		
		void set_image(const Data& image_data)
		{
			const TimeFrequencyData& source = image_data._tfData;
			const size_t imageCount = source.ImageCount();
			if(imageCount != _tfData.ImageCount())
			{
				std::ostringstream s;
				s << "set_image() was executed with incompatible polarizations: input had " << imageCount << ", destination had " << _tfData.ImageCount();
				throw BadUsageException(s.str());
			}
			for(size_t i=0; i!=imageCount; ++i)
				_tfData.SetImage(i, source.GetImage(i));
		}
		
		void set_polarization_data(PolarizationEnum polarization, const Data& data)
		{
			size_t polIndex = _tfData.GetPolarizationIndex(polarization);
			_tfData.SetPolarizationData(polIndex, data._tfData);
		}
		
		TimeFrequencyData& TFData() { return _tfData; }
		const TimeFrequencyData& TFData() const { return _tfData; }
		
	private:
		TimeFrequencyData _tfData;
	};
}

#endif
