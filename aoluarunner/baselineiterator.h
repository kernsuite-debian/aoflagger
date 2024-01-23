#ifndef RFISTRATEGYFOREACHBASELINEACTION_H
#define RFISTRATEGYFOREACHBASELINEACTION_H

#include "options.h"

#include "../imagesets/imageset.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <stack>
#include <string>
#include <thread>
#include <utility>

class BaselineIterator {
 public:
  BaselineIterator(std::mutex* ioMutex, const Options& options);
  ~BaselineIterator();

  void Run(imagesets::ImageSet& imageSet, class LuaThreadGroup& lua,
           class ScriptData& scriptData);

 private:
  bool IsSequenceSelected(imagesets::ImageSetIndex& index);
  imagesets::ImageSetIndex GetNextIndex();
  static std::string memToStr(double memSize);

  void SetExceptionOccured();
  void SetFinishedBaselines();
  // void SetProgress(ProgressListener &progress, int no, int count, const
  // std::string& taskName, int threadId);

  size_t BaselineProgress() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _baselineProgress;
  }
  void IncBaselineProgress() {
    std::lock_guard<std::mutex> lock(_mutex);
    ++_baselineProgress;
  }

  void WaitForReadBufferAvailable(size_t maxSize) {
    std::unique_lock<std::mutex> lock(_mutex);
    while (_baselineBuffer.size() > maxSize && !_exceptionOccured)
      _dataProcessed.wait(lock);
  }

  std::unique_ptr<imagesets::BaselineData> GetNextBaseline() {
    std::unique_lock<std::mutex> lock(_mutex);
    while (_baselineBuffer.size() == 0 && !_exceptionOccured &&
           !_finishedBaselines)
      _dataAvailable.wait(lock);
    if ((_finishedBaselines && _baselineBuffer.size() == 0) ||
        _exceptionOccured) {
      return nullptr;
    } else {
      std::unique_ptr<imagesets::BaselineData> next =
          std::move(_baselineBuffer.top());
      _baselineBuffer.pop();
      _dataProcessed.notify_one();
      return next;
    }
  }

  size_t GetBaselinesInBufferCount() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _baselineBuffer.size();
  }

  struct ProcessingThread {
    ProcessingThread(BaselineIterator& parent, size_t threadIndex)
        : _parent(parent), _threadIndex(threadIndex) {}
    BaselineIterator& _parent;
    size_t _threadIndex;
    void operator()();
  };

  struct ReaderThread {
    explicit ReaderThread(BaselineIterator& parent) : _parent(parent) {}
    void operator()();

    BaselineIterator& _parent;
  };

  const Options& _options;
  LuaThreadGroup* _lua;
  imagesets::ImageSet* _imageSet;
  size_t _sequenceCount, _nextIndex;
  size_t _threadCount;

  imagesets::ImageSetIndex _loopIndex;

  std::unique_ptr<class WriteThread> _writeThread;

  std::mutex _mutex, *_ioMutex;
  std::condition_variable _dataAvailable, _dataProcessed;
  std::stack<std::unique_ptr<imagesets::BaselineData>> _baselineBuffer;
  bool _finishedBaselines;

  bool _exceptionOccured;
  size_t _baselineProgress;
  class ScriptData* _globalScriptData;
};

#endif
