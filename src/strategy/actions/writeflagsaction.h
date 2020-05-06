#ifndef WRITEFLAGSACTION_H
#define WRITEFLAGSACTION_H

#include "action.h"

#include "../imagesets/imageset.h"

#include <condition_variable>
#include <stack>
#include <mutex>
#include <thread>
#include <memory>

#include "../../structures/mask2d.h"

namespace rfiStrategy {

	class WriteFlagsAction : public Action {
		public:
			WriteFlagsAction();
			virtual ~WriteFlagsAction();

			virtual std::string Description() final override
			{
				return "Write flags to file";
			}

			virtual void Perform(class ArtifactSet &artifacts, ProgressListener &progress) final override;
			virtual ActionType Type() const final override { return WriteFlagsActionType; }
			virtual void Finish() final override;
			virtual void Sync() final override { Finish(); Initialize(); }

			void SetMaxBufferItems(size_t maxBufferItems) { _maxBufferItems = maxBufferItems; }
			void SetMinBufferItemsForWriting(size_t minBufferItemsForWriting) { _minBufferItemsForWriting = minBufferItemsForWriting; }
		private:
			struct BufferItem {
				BufferItem(const std::vector<Mask2DCPtr> &masks, const ImageSetIndex &index)
					: _masks(masks), _index(index.Clone())
				{
				}
				BufferItem(const BufferItem &source) : _masks(source._masks), _index(source._index->Clone())
				{
				}
				~BufferItem()
				{
				}
				void operator=(const BufferItem &source)
				{
					_masks = source._masks;
					_index = source._index->Clone();
				}
				std::vector<Mask2DCPtr> _masks;
				std::unique_ptr<ImageSetIndex> _index;
			};

			struct FlushFunction
			{
				WriteFlagsAction *_parent;
				void operator()();
			};

			void pushInBuffer(const BufferItem &newItem)
			{
				std::unique_lock<std::mutex> lock(_mutex);
				while(_buffer.size() >= _maxBufferItems)
					_bufferChange.wait(lock);
				_buffer.emplace(newItem);
				_bufferChange.notify_all();
			}

			std::mutex _mutex;
			std::mutex *_ioMutex;
			std::condition_variable _bufferChange;
			std::unique_ptr<std::thread> _flusher;
			bool _isFinishing;

			size_t _maxBufferItems;
			size_t _minBufferItemsForWriting;

			std::stack<BufferItem> _buffer;
			std::unique_ptr<ImageSet> _imageSet;
	};
}
#endif
