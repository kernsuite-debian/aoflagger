#include "writethread.h"

#include "../imagesets/multibandmsimageset.h"
#include "../util/logger.h"

WriteThread::WriteThread(imagesets::ImageSet& imageSet, size_t calcThreadCount,
                         std::mutex* ioMutex)
    : _ioMutex(ioMutex),
      _isWriteFinishing(false),
      _maxWriteBufferItems(calcThreadCount * 5),
      _minWriteBufferItemsForWriting(calcThreadCount * 4) {
  FlushThread flushFunction;
  flushFunction._parent = this;
  if (dynamic_cast<imagesets::MultiBandMsImageSet*>(&imageSet)) {
    // TODO Would this method also be safe for other writers?
    _flusher.reset(new std::thread(flushFunction, &imageSet));
  } else {
    std::unique_lock<std::mutex> iolock(*_ioMutex);
    std::unique_ptr<imagesets::ImageSet> localImageSet = imageSet.Clone();
    iolock.unlock();
    _flusher.reset(new std::thread(flushFunction, std::move(localImageSet)));
  }
}

WriteThread::~WriteThread() {
  std::unique_lock<std::mutex> lock(_writeMutex);
  _isWriteFinishing = true;
  _writeBufferChange.notify_all();
  lock.unlock();

  Logger::Debug << "Finishing the flusher thread...\n";
  _flusher->join();
}

void WriteThread::SaveFlags(const TimeFrequencyData& data,
                            imagesets::ImageSetIndex& imageSetIndex) {
  std::vector<Mask2DCPtr> masks;
  if (data.MaskCount() <= 1)
    masks.emplace_back(data.GetSingleMask());
  else
    for (size_t i = 0; i < data.MaskCount(); ++i) {
      masks.emplace_back(data.GetMask(i));
    }
  BufferItem newItem(masks, imageSetIndex);
  pushInWriteBuffer(newItem);
}

void WriteThread::pushInWriteBuffer(const BufferItem& newItem) {
  std::unique_lock<std::mutex> lock(_writeMutex);
  while (_writeBuffer.size() >= _maxWriteBufferItems)
    _writeBufferChange.wait(lock);
  _writeBuffer.emplace(newItem);
  _writeBufferChange.notify_all();
}

void WriteThread::FlushThread::operator()(
    std::unique_ptr<imagesets::ImageSet> imageSet) {
  operator()(imageSet.get());
}

void WriteThread::FlushThread::operator()(imagesets::ImageSet* imageSet) {
  std::unique_lock<std::mutex> lock(_parent->_writeMutex);
  do {
    while (_parent->_writeBuffer.size() <
               _parent->_minWriteBufferItemsForWriting &&
           !_parent->_isWriteFinishing)
      _parent->_writeBufferChange.wait(lock);

    std::stack<BufferItem> bufferCopy;
    while (!_parent->_writeBuffer.empty()) {
      BufferItem item = _parent->_writeBuffer.top();
      _parent->_writeBuffer.pop();
      bufferCopy.push(item);
    }
    _parent->_writeBufferChange.notify_all();
    if (bufferCopy.size() >= _parent->_minWriteBufferItemsForWriting)
      Logger::Debug << "Flag buffer has reached minimal writing size, flushing "
                       "flags...\n";
    else
      Logger::Debug << "Flushing flags...\n";
    lock.unlock();

    std::unique_lock<std::mutex> ioLock(*_parent->_ioMutex);
    while (!bufferCopy.empty()) {
      BufferItem item = bufferCopy.top();
      bufferCopy.pop();
      imageSet->AddWriteFlagsTask(item._index, item._masks);
    }
    imageSet->PerformWriteFlagsTask();
    ioLock.unlock();

    lock.lock();
  } while (!_parent->_isWriteFinishing || !_parent->_writeBuffer.empty());
}
