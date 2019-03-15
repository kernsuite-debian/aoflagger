#ifndef SPATIALTIMEIMAGESET_H
#define SPATIALTIMEIMAGESET_H

#include <string>
#include <cstring>
#include <sstream>
#include <stack>
#include <stdexcept>

#include "imageset.h"

#include "../../structures/measurementset.h"
#include "../../msio/spatialtimeloader.h"
#include "../control/defaultstrategy.h"

namespace rfiStrategy {

	class SpatialTimeImageSetIndex : public ImageSetIndex {
		public:
			friend class SpatialTimeImageSet;

			explicit SpatialTimeImageSetIndex(ImageSet &set) : ImageSetIndex(set), _channelIndex(0), _isValid(true)
			{
			}
			inline virtual void Previous() final override;
			inline virtual void Next() final override;
			virtual std::string Description() const final override
			{
				std::stringstream s;
				s << "Channel " << _channelIndex;
				return s.str();
			}
			virtual bool IsValid() const final override
			{
				return _isValid;
			}
			virtual std::unique_ptr<ImageSetIndex> Clone() const final override
			{
				std::unique_ptr<SpatialTimeImageSetIndex>
					newIndex( new SpatialTimeImageSetIndex(imageSet()) );
				newIndex->_channelIndex = _channelIndex;
				newIndex->_isValid = _isValid;
				return std::move(newIndex);
			}
		private:
			inline class SpatialTimeImageSet &STMSSet() const;
			size_t _channelIndex;
			bool _isValid;
	};
	
	class SpatialTimeImageSet : public ImageSet {
	public:
		explicit SpatialTimeImageSet(const std::string &location) : _set(location), _loader(_set)
		{
		}
		virtual ~SpatialTimeImageSet()
		{
		}
		virtual std::unique_ptr<ImageSet> Clone() final override
		{
			return nullptr;
		}

		virtual std::unique_ptr<ImageSetIndex> StartIndex() final override
		{
			return std::unique_ptr<ImageSetIndex>(new SpatialTimeImageSetIndex(*this));
		}
		virtual void Initialize() final override
		{
		}
		virtual std::string Name() final override
		{
			return "Spatial time matrix";
		}
		virtual std::string File() final override
		{
			return _set.Path(); 
		}
		virtual std::string TelescopeName() final override
		{
			return DefaultStrategy::TelescopeName(DefaultStrategy::GENERIC_TELESCOPE);
		}			
		size_t GetTimeIndexCount()
		{
			return _loader.TimestepsCount();
		}
		size_t GetFrequencyCount()
		{
			return _loader.ChannelCount();
		}
		virtual void AddReadRequest(const ImageSetIndex &index) final override
		{
			_baseline.push(BaselineData(index));
		}
		virtual void PerformReadRequests() final override
		{
			TimeFrequencyData *data = loadData(_baseline.top().Index());
			_baseline.top().SetData(*data);
			_baseline.top().SetMetaData(TimeFrequencyMetaDataPtr());
			delete data;
		}
		virtual std::unique_ptr<BaselineData> GetNextRequested() final override
		{
			std::unique_ptr<BaselineData> data(new BaselineData(_baseline.top()));
			_baseline.pop();
			return std::move(data);
		}
		virtual void AddWriteFlagsTask(const ImageSetIndex &, std::vector<Mask2DCPtr> &) final override
		{
			throw std::runtime_error("Not implemented");
		}
		virtual void PerformWriteFlagsTask() final override
		{
			throw std::runtime_error("Not implemented");
		}
		virtual void PerformWriteDataTask(const ImageSetIndex &, std::vector<Image2DCPtr>, std::vector<Image2DCPtr>) final override
		{
			throw std::runtime_error("Not implemented");
		}
	private:
		MeasurementSet _set;
		SpatialTimeLoader _loader;
		std::stack<BaselineData> _baseline;
		
		virtual TimeFrequencyData *loadData(const ImageSetIndex &index)
		{
			const SpatialTimeImageSetIndex &sIndex = static_cast<const SpatialTimeImageSetIndex&>(index);
			TimeFrequencyData *result = new TimeFrequencyData(_loader.Load(sIndex._channelIndex));
			return result;
		}
	};

	void SpatialTimeImageSetIndex::Previous()
	{
		if(_channelIndex > 0)
			--_channelIndex;
		else
		{
			_channelIndex = STMSSet().GetFrequencyCount()-1;
			_isValid = false;
		}
	}

	void SpatialTimeImageSetIndex::Next()
	{
		++_channelIndex;
		if(_channelIndex == STMSSet().GetFrequencyCount())
		{
			_channelIndex = 0;
			_isValid = false;
		}
	}

	class SpatialTimeImageSet &SpatialTimeImageSetIndex::STMSSet() const
	{
		return static_cast<SpatialTimeImageSet&>(imageSet());
	}
}

#endif
