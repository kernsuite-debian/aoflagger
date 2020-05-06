#ifndef PYTHON_DATA_H
#define PYTHON_DATA_H

#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

#include <boost/python/list.hpp>

namespace aoflagger_python
{
	class Data
	{
	public:
		Data() = default;
		
		Data(const TimeFrequencyData& tfData, TimeFrequencyMetaDataCPtr metaData) :
			_tfData(tfData),
			_metaData(metaData)
		{ }
		
		Data(TimeFrequencyData&& tfData, TimeFrequencyMetaDataCPtr metaData) noexcept :
			_tfData(std::move(tfData)),
			_metaData(metaData)
		{ }
		
		Data operator-(const Data& other) const
		{
			return Data(TimeFrequencyData::MakeFromDiff(_tfData, other.TFData()), _metaData);
		}
		
		void clear_mask() noexcept
		{
			_tfData.SetNoMask();
		}
		
		Data convert_to_polarization(PolarizationEnum polarization) const
		{
			return Data(_tfData.Make(polarization), _metaData);
		}
		
		Data convert_to_complex(enum TimeFrequencyData::ComplexRepresentation complexRepresentation) const
		{
			return Data(_tfData.Make(complexRepresentation), _metaData);
		}
		
		Data copy() const
		{
			return Data(_tfData, _metaData);
		}
		
		void join_mask(const Data& other)
		{
			_tfData.JoinMask(other._tfData);
		}
		
		Data make_complex() const
		{
			return Data(_tfData.MakeFromComplexCombination(_tfData, _tfData), _metaData);
		}
		
		boost::python::list polarizations() const
		{
			const std::vector<PolarizationEnum> pols = _tfData.Polarizations();
			boost::python::list polList;
			for (PolarizationEnum p : pols)
				polList.append(p);
			return polList;
		}
		
		void set_visibilities(const Data& image_data)
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
		
		TimeFrequencyData& TFData() noexcept { return _tfData; }
		const TimeFrequencyData& TFData() const noexcept { return _tfData; }
		
		const TimeFrequencyMetaDataCPtr& MetaData() const noexcept { return _metaData; }
		
	private:
		TimeFrequencyData _tfData;
		TimeFrequencyMetaDataCPtr _metaData;
	};
}

#endif
