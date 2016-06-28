#include "writeflagsaction.h"

#include <iostream>

#include "../../util/aologger.h"

#include "../control/artifactset.h"

#include "../imagesets/imageset.h"

#include <boost/thread.hpp>

namespace rfiStrategy {

	WriteFlagsAction::WriteFlagsAction() : _flusher(0), _isFinishing(false), _maxBufferItems(18), _minBufferItemsForWriting(12), _imageSet(0)
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

		boost::mutex::scoped_lock lock(_mutex);
		if(_flusher == 0)
		{
			_ioMutex = &artifacts.IOMutex();
			boost::mutex::scoped_lock iolock(*_ioMutex);
			_imageSet = artifacts.ImageSet()->Copy();
			iolock.unlock();
			_isFinishing = false;
			FlushFunction flushFunction;
			flushFunction._parent = this;
			_flusher = new boost::thread(flushFunction);
		}
		lock.unlock();

		std::vector<Mask2DCPtr> masks;
		for(size_t i=0;i<artifacts.ContaminatedData().MaskCount();++i)
		{
			Mask2DCPtr mask = artifacts.ContaminatedData().GetMask(i);
			masks.push_back(mask);
		}
		BufferItem newItem(masks, *artifacts.ImageSetIndex());
		pushInBuffer(newItem);
	}

	void WriteFlagsAction::FlushFunction::operator()()
	{
		boost::mutex::scoped_lock lock(_parent->_mutex);
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
				AOLogger::Debug << "Flag buffer has reached minimal writing size, flushing flags...\n";
			else
				AOLogger::Debug << "Flushing flags...\n";
			lock.unlock();

			boost::mutex::scoped_lock ioLock(*_parent->_ioMutex);
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
		boost::mutex::scoped_lock lock(_mutex);
		_isFinishing = true;
		_bufferChange.notify_all();
		if(_flusher != 0)
		{
			boost::thread *flusher = _flusher;
			_flusher = 0;
			lock.unlock();
			AOLogger::Debug << "Finishing the flusher thread...\n";
			flusher->join();
			delete flusher;
			delete _imageSet;
		}
	}
}
