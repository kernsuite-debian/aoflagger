#ifndef AOLUA_WRITE_THREAD_H
#define AOLUA_WRITE_THREAD_H

#include "../imagesets/imageset.h"

#include <condition_variable>
#include <stack>
#include <thread>
#include <vector>

class WriteThread {
 public:
  WriteThread(rfiStrategy::ImageSet& imageSet, size_t calcThreadCount,
              std::mutex* ioMutex);
  ~WriteThread();

  void SaveFlags(const TimeFrequencyData& data,
                 rfiStrategy::ImageSetIndex& imageSetIndex);

 private:
  struct FlushThread {
    WriteThread* _parent;
    void operator()(std::unique_ptr<rfiStrategy::ImageSet> imageSet);
  };

  struct BufferItem {
    BufferItem(const std::vector<Mask2DCPtr>& masks,
               const rfiStrategy::ImageSetIndex& index)
        : _masks(masks), _index(index) {}
    std::vector<Mask2DCPtr> _masks;
    rfiStrategy::ImageSetIndex _index;
  };

  void pushInWriteBuffer(const BufferItem& newItem);

  std::mutex _writeMutex, *_ioMutex;
  std::condition_variable _writeBufferChange;
  std::unique_ptr<std::thread> _flusher;
  bool _isWriteFinishing;

  size_t _maxWriteBufferItems;
  size_t _minWriteBufferItemsForWriting;

  std::stack<BufferItem> _writeBuffer;
};

#endif
