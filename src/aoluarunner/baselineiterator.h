#ifndef RFISTRATEGYFOREACHBASELINEACTION_H
#define RFISTRATEGYFOREACHBASELINEACTION_H

#include "options.h"

#include "../strategy/imagesets/imageset.h"
#include "../strategy/control/types.h"

#include <stack>
#include <set>
#include <mutex>
#include <condition_variable>
#include <thread>

class BaselineIterator {
public:
	BaselineIterator(std::mutex* ioMutex, const Options& options);
	~BaselineIterator();
	
	void Run(rfiStrategy::ImageSet& imageSet, class LuaThreadGroup& lua);
	
private:
	bool IsBaselineSelected(rfiStrategy::ImageSetIndex &index);
	std::unique_ptr<rfiStrategy::ImageSetIndex> GetNextIndex();
	static std::string memToStr(double memSize);
	
	void SetExceptionOccured();
	void SetFinishedBaselines();
	//void SetProgress(ProgressListener &progress, int no, int count, const std::string& taskName, int threadId);

	size_t BaselineProgress()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		return _baselineProgress;
	}
	void IncBaselineProgress()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		++_baselineProgress;
	}
	
	void WaitForReadBufferAvailable(size_t maxSize)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		while(_baselineBuffer.size() > maxSize && !_exceptionOccured)
			_dataProcessed.wait(lock);
	}
	
	std::unique_ptr<rfiStrategy::BaselineData> GetNextBaseline()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		while(_baselineBuffer.size() == 0 && !_exceptionOccured && !_finishedBaselines)
			_dataAvailable.wait(lock);
		if((_finishedBaselines && _baselineBuffer.size() == 0) || _exceptionOccured)
			return nullptr;
		else
		{
			std::unique_ptr<rfiStrategy::BaselineData> next = std::move(_baselineBuffer.top());
			_baselineBuffer.pop();
			_dataProcessed.notify_one();
			return next;
		}
	}

	size_t GetBaselinesInBufferCount()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		return _baselineBuffer.size();
	}
	
	struct ProcessingThread
	{
		ProcessingThread(BaselineIterator& parent, size_t threadIndex)
			: _parent(parent), _threadIndex(threadIndex)
		{
		}
		BaselineIterator& _parent;
		size_t _threadIndex;
		void operator()();
	};
	
	struct ReaderThread
	{
		explicit ReaderThread(BaselineIterator& parent)
			: _parent(parent)
		{
		}
		void operator()();

		BaselineIterator& _parent;
	};
	
	const Options& _options;
	LuaThreadGroup* _lua;
	rfiStrategy::ImageSet* _imageSet;
	size_t _baselineCount, _nextIndex;
	size_t _threadCount;

	std::unique_ptr<rfiStrategy::ImageSetIndex> _loopIndex;
	
	std::unique_ptr<class WriteThread> _writeThread;
	
	std::mutex _mutex, *_ioMutex;
	std::condition_variable _dataAvailable, _dataProcessed;
	std::stack<std::unique_ptr<rfiStrategy::BaselineData>> _baselineBuffer;
	bool _finishedBaselines;

	bool _exceptionOccured;
	size_t _baselineProgress;
};

#endif

