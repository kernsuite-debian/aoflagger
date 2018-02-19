#include "writeflagsaction.h"

#include <iostream>
#include <thread>

#include "../../util/logger.h"

#include "../control/artifactset.h"

#include "../imagesets/imageset.h"


namespace rfiStrategy {

	WriteFlagsAction::WriteFlagsAction() : _ioMutex(nullptr), _flusher(), _isFinishing(false), _maxBufferItems(18), _minBufferItemsForWriting(12), _imageSet()
	{
	}
	
	
	WriteFlagsAction::~WriteFlagsAction()
	{
		Finish();
	}
	
	void WriteFlagsAction::Perform(class ArtifactSet &artifacts, ProgressListener &)
	{
		if(!artifacts.HasImageSet())
			throw BadUsageException("No image set active: can not write flags");

		std::unique_lock<std::mutex> lock(_mutex);
		if(_flusher == nullptr)
		{
			_ioMutex = &artifacts.IOMutex();
			std::unique_lock<std::mutex> iolock(*_ioMutex);
			_imageSet = artifacts.ImageSet().Clone();
			iolock.unlock();
			_isFinishing = false;
			FlushFunction flushFunction;
			flushFunction._parent = this;
			_flusher.reset( new std::thread(flushFunction) );
		}
		lock.unlock();

		std::vector<Mask2DCPtr> masks;
		for(size_t i=0;i<artifacts.ContaminatedData().MaskCount();++i)
		{
			Mask2DCPtr mask = artifacts.ContaminatedData().GetMask(i);
			masks.push_back(mask);
		}
		BufferItem newItem(masks, artifacts.ImageSetIndex());
		pushInBuffer(newItem);
	}

	void WriteFlagsAction::FlushFunction::operator()()
	{
		std::unique_lock<std::mutex> lock(_parent->_mutex);
		do {
			while(_parent->_buffer.size() < _parent->_minBufferItemsForWriting && !_parent->_isFinishing)
				_parent->_bufferChange.wait(lock);

			std::stack<BufferItem> bufferCopy;
			while(!_parent->_buffer.empty())
			{
				BufferItem item = _parent->_buffer.top();
				_parent->_buffer.pop();
				item._index->Reattach(*_parent->_imageSet);
				bufferCopy.push(item);
			}
			_parent->_bufferChange.notify_all();
			if(bufferCopy.size() >= _parent->_minBufferItemsForWriting)
				Logger::Debug << "Flag buffer has reached minimal writing size, flushing flags...\n";
			else
				Logger::Debug << "Flushing flags...\n";
			lock.unlock();

			std::unique_lock<std::mutex> ioLock(*_parent->_ioMutex);
			while(!bufferCopy.empty())
			{
				BufferItem item = bufferCopy.top();
				bufferCopy.pop();
				_parent->_imageSet->AddWriteFlagsTask(*item._index, item._masks);
			}
			_parent->_imageSet->PerformWriteFlagsTask();
			ioLock.unlock();

			lock.lock();
		} while(!_parent->_isFinishing || !_parent->_buffer.empty());
	}

	void WriteFlagsAction::Finish()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_isFinishing = true;
		_bufferChange.notify_all();
		if(_flusher != nullptr)
		{
			std::unique_ptr<std::thread> flusher = std::move(_flusher);
			lock.unlock();
			Logger::Debug << "Finishing the flusher thread...\n";
			flusher->join();
			flusher.reset();
			_imageSet.reset();
		}
	}
}
