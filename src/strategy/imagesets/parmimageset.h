#ifndef PARM_IMAGE_H
#define PARM_IMAGE_H

#include <string>
#include <cstring>
#include <vector>
#include <deque>

#include "../../structures/types.h"
#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"

#include "../control/defaultstrategy.h"

#include "imageset.h"

namespace rfiStrategy {

	class ParmImageSet;
	
	class ParmImageSetIndex : public ImageSetIndex {
		public:
			ParmImageSetIndex(ImageSet &set) : ImageSetIndex(set), _valid(true), _antennaIndex(0)
			{
			}
			virtual ~ParmImageSetIndex()
			{
			}
			inline virtual void Previous() final override;
			
			inline virtual void Next() final override;
			
			inline virtual std::string Description() const final override;
			
			virtual bool IsValid() const final override { return _valid; }
			
			virtual std::unique_ptr<ImageSetIndex> Clone() const final override
			{
				std::unique_ptr<ParmImageSetIndex> index( new ParmImageSetIndex(imageSet()) );
				index->_antennaIndex = _antennaIndex;
				return std::move(index);
			}
			
			unsigned AntennaIndex() const { return _antennaIndex; }
			
		private:
			inline ParmImageSet &ParmSet() const;
			bool _valid;
			unsigned _antennaIndex;
	};
	
	class ParmImageSet : public ImageSet {
		public:
			ParmImageSet(const std::string &path) : _path(path), _parmTable(nullptr)
			{
			}
			virtual ~ParmImageSet() override;
			virtual std::unique_ptr<ImageSet> Clone() final override
			{
				throw std::runtime_error("Cannot copy set");
			}
			virtual std::unique_ptr<ImageSetIndex> StartIndex() final override
			{
				return std::unique_ptr<ImageSetIndex>(new ParmImageSetIndex(*this));
			}
			virtual void Initialize() final override;
			
			virtual std::string Name() final override { return "Parmdb"; }
			
			virtual std::string File() final override { return _path; }
			
			virtual std::string TelescopeName() final override
			{
				return DefaultStrategy::TelescopeName(DefaultStrategy::GENERIC_TELESCOPE);
			}
	
			TimeFrequencyData *LoadData(const ImageSetIndex &index);
			
			virtual void AddReadRequest(const ImageSetIndex &index) final override
			{
				TimeFrequencyData *data = LoadData(index);
				BaselineData *baseline = new BaselineData(*data, TimeFrequencyMetaDataCPtr(), index);
				delete data;
				_baselineBuffer.push_back(baseline);
			}
			virtual void PerformReadRequests() final override
			{
			}
			
			virtual std::unique_ptr<BaselineData> GetNextRequested() final override
			{
				std::unique_ptr<BaselineData> baseline(std::move(_baselineBuffer.front()));
				_baselineBuffer.pop_front();
				return std::move(baseline);
			}

			unsigned AntennaCount() const
			{
				return _antennas.size();
			}
			std::string AntennaName(unsigned index) const { return _antennas[index]; }
		private:
			const std::string _path;
			std::vector<std::string> _antennas;
			class ParmTable *_parmTable;
			std::deque<BaselineData*> _baselineBuffer;
	};

	void ParmImageSetIndex::Previous()
	{
		if(_antennaIndex > 0)
			--_antennaIndex;
		else
		{
			_antennaIndex = ParmSet().AntennaCount() - 1;
			_valid = false;
		}
	}
	
	void ParmImageSetIndex::Next()
	{
		++_antennaIndex;
		if(_antennaIndex >= ParmSet().AntennaCount())
		{
			_antennaIndex = 0;
			_valid = false;
		}
	}
	
	ParmImageSet &ParmImageSetIndex::ParmSet() const
	{
		return static_cast<ParmImageSet&>(imageSet());
	}

	std::string ParmImageSetIndex::Description() const
	{
		return ParmSet().AntennaName(_antennaIndex);
	}
}

#endif
