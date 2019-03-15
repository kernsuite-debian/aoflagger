#ifndef QUALITY_STAT_IMAGE_SET_H
#define QUALITY_STAT_IMAGE_SET_H

#include "singleimageset.h"

#include "../../quality/qualitytablesformatter.h"
#include "../../quality/statisticsderivator.h"
#include "../../util/logger.h"
#include "../control/defaultstrategy.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

#include <boost/filesystem/path.hpp>

namespace rfiStrategy
{

class QualityStatImageSet : public SingleImageSet
{
public:
	explicit QualityStatImageSet(const std::string& filename) :
		_filename(filename),
		_statisticKind(QualityTablesFormatter::StandardDeviationStatistic)
	{
		if(!_filename.empty() && (*_filename.rbegin()) == '/')
			_filename.resize(_filename.size()-1);
		boost::filesystem::path p(_filename);
		if(p.filename() == "QUALITY_TIME_STATISTIC")
			_filename = p.parent_path().string();
	}
	
	virtual std::string Name() final override
	{ return File(); }
	virtual std::string File() final override
	{ return _filename; }
	virtual std::string BaselineDescription() final override
	{ return File(); }
	virtual std::string TelescopeName() final override
	{
		return DefaultStrategy::TelescopeName(DefaultStrategy::GENERIC_TELESCOPE);
	}

	std::unique_ptr<BaselineData> Read() final override
	{
		QualityTablesFormatter formatter(_filename);
		StatisticsCollection statCollection;
		statCollection.Load(formatter);

		StatisticsDerivator derivator(statCollection);
		
		std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr> data =
			derivator.CreateTFData(_statisticKind);
			
		TimeFrequencyData& tfData = data.first;
		TimeFrequencyMetaDataCPtr& metaData = data.second;
		Mask2DPtr mask = Mask2D::CreateUnsetMaskPtr(tfData.ImageWidth(), tfData.ImageHeight());
		for(size_t y=0; y!=tfData.ImageHeight(); ++y)
		{
			for(size_t x=0; x!=tfData.ImageWidth(); ++x)
			{
				for(size_t i=0; i!=tfData.ImageCount(); ++i)
					mask->SetValue(x, y, !std::isfinite(tfData.GetImage(i)->Value(x, y)));
			}
		}
		tfData.SetGlobalMask(mask);

		return std::unique_ptr<BaselineData>(new BaselineData(tfData, metaData));
	}
	
	virtual std::unique_ptr<ImageSet> Clone() final override
	{
		return std::unique_ptr<ImageSet>(new QualityStatImageSet(_filename));
	}
	
	virtual void Initialize() final override
	{ }
	
	virtual void Write(const std::vector<Mask2DCPtr>& flags) final override
	{
		std::vector<Mask2DCPtr> flagsCopy(flags);
		casacore::MeasurementSet ms(_filename, casacore::Table::Update);
		if(ms.nrow() > 0)
		{
			casacore::ArrayColumn<bool> flagColumn(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FLAG));
			casacore::ROScalarColumn<double> timeColumn(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::TIME));
			casacore::Array<bool> flagArray(flagColumn.get(0));
			const size_t
				polCount = flagArray.shape()[0],
				channelCount = flagArray.shape()[1];
			Logger::Debug << "Saving flags to measurement set (" << channelCount << " ch x " << polCount << " pol)...\n";
			if(flagsCopy.size() == 1 && polCount>1)
			{
				do { flagsCopy.push_back(flagsCopy[0]);
				} while(flagsCopy.size() != polCount);
			}
			if(flagsCopy.size() != polCount)
				throw std::runtime_error("Polarization counts don't match");
			if(flagsCopy[0]->Height() != channelCount)
			{
				std::ostringstream s;
				s << "Channel counts don't match (" << flagsCopy[0]->Height() << " in mask, " << channelCount << " in data)";
				throw std::runtime_error(s.str());
			}
			size_t timeIndex = size_t(-1);
			double time = -1.0;
			std::unique_ptr<bool[]> timestepFlags(new bool[channelCount*polCount]);
			for(size_t row=0; row!=ms.nrow(); ++row)
			{
				if(time != timeColumn(row))
				{
					time = timeColumn(row);
					timeIndex++;
					
					bool* iter = timestepFlags.get();
					for(size_t ch=0; ch!=channelCount; ++ch)
					{
						for(size_t p=0; p!=polCount; ++p)
						{
							*iter = flagsCopy[p]->Value(timeIndex, ch);
							++iter;
						}
					}
				}
				flagColumn.get(row, flagArray);
				casacore::Array<bool>::contiter iter = flagArray.cbegin();
				for(size_t i=0; i!=channelCount*polCount; ++i)
				{
					*iter = *iter || timestepFlags[i];
					++iter;
				}
				flagColumn.put(row, flagArray);
			}
			timestepFlags.reset();
		}
	}
	
private:
	std::string _filename;
	QualityTablesFormatter::StatisticKind _statisticKind;
};

} // namespace

#endif
