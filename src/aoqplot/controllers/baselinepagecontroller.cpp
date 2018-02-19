#include "baselinepagecontroller.h"

#include "../../quality/statisticscollection.h"
#include "../../quality/statisticsderivator.h"

std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr> BaselinePageController::constructImage(QualityTablesFormatter::StatisticKind kind)
{
	if(HasStatistics())
	{
		const unsigned polarizationCount = _statCollection->PolarizationCount();
		std::vector<std::pair<unsigned, unsigned> > baselines = _statCollection->BaselineStatistics().BaselineList();
		StatisticsDerivator derivator(*_statCollection);
		
		const unsigned antennaCount = _statCollection->BaselineStatistics().AntennaCount();

		std::vector<Image2DPtr> realImages(polarizationCount);
		std::vector<Image2DPtr> imagImages(polarizationCount);
		std::vector<Mask2DPtr> mask(polarizationCount);
		for(unsigned p=0;p<polarizationCount;++p)
		{
			realImages[p] = Image2D::CreateUnsetImagePtr(antennaCount, antennaCount);
			realImages[p]->SetAll(std::numeric_limits<num_t>::quiet_NaN());
			imagImages[p] = Image2D::CreateUnsetImagePtr(antennaCount, antennaCount);
			imagImages[p]->SetAll(std::numeric_limits<num_t>::quiet_NaN());
			mask[p] = Mask2D::CreateSetMaskPtr<true>(antennaCount, antennaCount);
		}
		
		for(std::vector<std::pair<unsigned, unsigned> >::const_iterator i=baselines.begin();i!=baselines.end();++i)
		{
			const unsigned antenna1 = i->first, antenna2 = i->second;
			for(unsigned p=0;p<polarizationCount;++p)
			{
				const std::complex<long double> val = derivator.GetComplexBaselineStatistic(kind, antenna1, antenna2, p);
				realImages[p]->SetValue(antenna1, antenna2, val.real());
				imagImages[p]->SetValue(antenna1, antenna2, val.imag());
				mask[p]->SetValue(antenna1, antenna2, false);
			}
		}
		TimeFrequencyData data;
		if(polarizationCount == 1)
		{
			data = TimeFrequencyData(Polarization::StokesI, realImages[0], imagImages[0]);
			data.SetGlobalMask(mask[0]);
		}
		else if(polarizationCount == 2)
		{
			data = TimeFrequencyData(Polarization::XX, realImages[0], imagImages[0],
															 Polarization::YY, realImages[1], imagImages[1]);
			data.SetIndividualPolarizationMasks(mask[0], mask[1]);
		}
		else if(polarizationCount == 4)
		{
			data = TimeFrequencyData::FromLinear(realImages[0], imagImages[0], realImages[1], imagImages[1], realImages[2], imagImages[2], realImages[3], imagImages[3]);
			data.SetIndividualPolarizationMasks(mask[0], mask[1], mask[2], mask[3]);
		}
		else {
			std::stringstream s;
			s << "Set has not 1, 2 or 4 polarizations (?!?) : StatisticsCollection.PolarizationCount() == " << polarizationCount;
			throw std::runtime_error(s.str());
		}
		return std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr>(data, TimeFrequencyMetaDataCPtr());
	} else {
		return std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr>(TimeFrequencyData(), TimeFrequencyMetaDataCPtr());
	}
}

std::string BaselinePageController::AntennaName(size_t index) const
{
	std::string name;
	if(_antennas == 0)
	{
		std::stringstream s;
		s << index;
		name = s.str();
	} else {
		name = (*_antennas)[index].name;
	}
	return name;
}

