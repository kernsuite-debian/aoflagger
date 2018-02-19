#ifndef SPATIALMSIMAGESET_H
#define SPATIALMSIMAGESET_H

#include <string>
#include <cstring>
#include <sstream>
#include <stack>
#include <stdexcept>

#include "imageset.h"

#include "../../structures/spatialmatrixmetadata.h"
#include "../../structures/measurementset.h"

#include "../../msio/baselinematrixloader.h"

namespace rfiStrategy {

	class SpatialMSImageSetIndex : public ImageSetIndex {
		public:
			friend class SpatialMSImageSet;

			explicit SpatialMSImageSetIndex(ImageSet &set) : ImageSetIndex(set), _timeIndex(0), _channelIndex(0), _isValid(true)
			{
			}
			inline virtual void Previous() final override;
			inline virtual void Next() final override;
			virtual std::string Description() const final override
			{
				std::stringstream s;
				s << "Time index " << _timeIndex << ", channel " << _channelIndex;
				return s.str();
			}
			virtual bool IsValid() const final override
			{
				return _isValid;
			}
			virtual std::unique_ptr<ImageSetIndex> Clone() const final override
			{
				std::unique_ptr<SpatialMSImageSetIndex> newIndex( new SpatialMSImageSetIndex(imageSet()) );
				newIndex->_timeIndex = _timeIndex;
				newIndex->_channelIndex = _channelIndex;
				newIndex->_isValid = _isValid;
				return std::move(newIndex);
			}
		private:
			inline class SpatialMSImageSet &SMSSet() const;
			size_t _timeIndex;
			size_t _channelIndex;
			bool _isValid;
	};
	
	class SpatialMSImageSet : public ImageSet {
		public:
			explicit SpatialMSImageSet(const std::string &location) : _set(location), _loader(_set), _cachedTimeIndex(GetTimeIndexCount())
			{
			}
			virtual ~SpatialMSImageSet()
			{
			}
			virtual std::unique_ptr<ImageSet> Clone() final override
			{
				return nullptr;
			}

			virtual std::unique_ptr<ImageSetIndex> StartIndex() final override
			{
				return std::unique_ptr<ImageSetIndex>(new SpatialMSImageSetIndex(*this));
			}
			virtual void Initialize() final override
			{
			}
			virtual std::string Name() final override
			{
				return "Spatial correlation matrix"; 
			}
			virtual std::string File() final override
			{
				return _set.Path(); 
			}
			virtual void LoadFlags(const ImageSetIndex &/*index*/, TimeFrequencyData &/*destination*/)
			{
			}
			virtual TimeFrequencyMetaDataCPtr LoadMetaData(const ImageSetIndex &/*index*/)
			{
				return TimeFrequencyMetaDataCPtr();
			}
			SpatialMatrixMetaData SpatialMetaData(const ImageSetIndex &index)
			{
				const SpatialMSImageSetIndex &sIndex = static_cast<const SpatialMSImageSetIndex&>(index);
				SpatialMatrixMetaData metaData(_loader.MetaData());
				metaData.SetChannelIndex(sIndex._channelIndex);
				metaData.SetTimeIndex(sIndex._timeIndex);
				return metaData;
			}
			virtual size_t GetPart(const ImageSetIndex &/*index*/)
			{
				return 0;
			}
			virtual size_t GetAntenna1(const ImageSetIndex &/*index*/)
			{
				return 0;
			}
			virtual size_t GetAntenna2(const ImageSetIndex &/*index*/)
			{
				return 0;
			}
			size_t GetTimeIndexCount()
			{
				return _loader.TimeIndexCount();
			}
			size_t GetFrequencyCount()
			{
				return _loader.FrequencyCount();
			}
			virtual void AddReadRequest(const ImageSetIndex &index) final override
			{
				_baseline.push(BaselineData(index));
			}
			virtual void PerformReadRequests() final override
			{
				TimeFrequencyData *data = LoadData(_baseline.top().Index());
				_baseline.top().SetData(*data);
				_baseline.top().SetMetaData(TimeFrequencyMetaDataPtr());
				delete data;
			}
			virtual std::unique_ptr<BaselineData> GetNextRequested() final override
			{
				std::unique_ptr<BaselineData> data(new BaselineData(_baseline.top()));
				_baseline.pop();
				return data;
			}
			
			TimeFrequencyData* LoadData(const ImageSetIndex &index)
			{
				const SpatialMSImageSetIndex &sIndex = static_cast<const SpatialMSImageSetIndex&>(index);
				if(sIndex._timeIndex != _cachedTimeIndex)
				{
					_loader.LoadPerChannel(sIndex._timeIndex, _timeIndexMatrices);
					_cachedTimeIndex = sIndex._timeIndex;
				}
				TimeFrequencyData *result = new TimeFrequencyData(_timeIndexMatrices[sIndex._channelIndex]);
				return result;
			}
		private:
			MeasurementSet _set;
			BaselineMatrixLoader _loader;
			std::stack<BaselineData> _baseline;
			std::vector<TimeFrequencyData> _timeIndexMatrices;
			size_t _cachedTimeIndex;
	};

	void SpatialMSImageSetIndex::Previous()
	{
		if(_channelIndex > 0)
			--_channelIndex;
		else
		{
			_channelIndex = SMSSet().GetFrequencyCount()-1;
			if(_timeIndex > 0)
				--_timeIndex;
			else
			{
				_timeIndex = SMSSet().GetTimeIndexCount()-1;
				_isValid = false;
			}
		}
	}

	void SpatialMSImageSetIndex::Next()
	{
		++_channelIndex;
		if(_channelIndex == SMSSet().GetFrequencyCount())
		{
			_channelIndex = 0;
			++_timeIndex;
			if(_timeIndex == SMSSet().GetTimeIndexCount())
			{
				_timeIndex = 0;
				_isValid = false;
			}
		}
	}

	class SpatialMSImageSet &SpatialMSImageSetIndex::SMSSet() const
	{
		return static_cast<SpatialMSImageSet&>(imageSet());
	}
}

#endif
