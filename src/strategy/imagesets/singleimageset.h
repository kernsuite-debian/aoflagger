#ifndef SINGLEIMAGESET_H
#define SINGLEIMAGESET_H

#include <string>
#include <stdexcept>

#include "../../structures/types.h"

#include "imageset.h"

namespace rfiStrategy {

	class SingleImageSetIndex : public ImageSetIndex {
		public:
			SingleImageSetIndex(ImageSet &set, const std::string& description) : ImageSetIndex(set), _valid(true), _description(description) { }
			virtual ~SingleImageSetIndex() { }
			virtual void Previous() final override { _valid = false; }
			virtual void Next() final override { _valid = false; }
			virtual std::string Description() const final override { return _description; }
			virtual bool IsValid() const final override { return _valid; }
			virtual std::unique_ptr<ImageSetIndex> Clone() const final override
			{
				std::unique_ptr<SingleImageSetIndex> index( new SingleImageSetIndex(imageSet(), _description) );
				index->_valid = _valid;
				return std::move(index);
			}
		private:
			bool _valid;
			std::string _description;
	};
	
	class SingleImageSet : public ImageSet {
		public:
			SingleImageSet() : ImageSet(), _readCount(0), _lastRead(nullptr), _writeFlagsIndex()
			{ }
			
			virtual std::unique_ptr<ImageSetIndex> StartIndex() override
			{
				return std::unique_ptr<ImageSetIndex>(new SingleImageSetIndex(*this, Name()));
			}
			
			virtual std::string Name() override = 0;
			virtual std::string File() override = 0;
			
			virtual void AddReadRequest(const ImageSetIndex &) override
			{
				if(_lastRead != nullptr)
				{
					_lastRead.reset();
					_readCount = 1;
				} else {
					++_readCount;
				}
			}
			virtual void PerformReadRequests() override
			{
				_lastRead = Read();
				_lastRead->SetIndex(SingleImageSetIndex(*this, Name()));
			}
			virtual std::unique_ptr<BaselineData> GetNextRequested() override
			{
				if(_readCount == 0)
					throw std::runtime_error("All data reads have already been requested");
				if(_lastRead == 0)
					throw std::runtime_error("GetNextRequested() was called before PerformReadRequests()");
				return std::unique_ptr<BaselineData>(new BaselineData(*_lastRead));
			}
			
			virtual std::unique_ptr<BaselineData> Read() = 0;
			
			virtual void Write(const std::vector<Mask2DCPtr>&)
			{
				throw std::runtime_error("Flag writing is not implemented for this file (SingleImageSet)");
			}
			
			virtual void AddWriteFlagsTask(const ImageSetIndex& index, std::vector<Mask2DCPtr>& flags) override
			{
				_writeFlagsIndex = index.Clone();
				_writeFlagsMasks = flags;
			}
			
			virtual void PerformWriteFlagsTask() override
			{
				if(_writeFlagsIndex == nullptr)
					throw std::runtime_error("Nothing to write");
				
				Write(_writeFlagsMasks);
				
				_writeFlagsIndex.reset();
			}
			
		private:
			int _readCount;
			std::unique_ptr<BaselineData> _lastRead;
			std::unique_ptr<ImageSetIndex> _writeFlagsIndex;
			std::vector<Mask2DCPtr> _writeFlagsMasks;
	};

}

#endif
