#ifndef UTIL_PROGRESS_SUB_TASK_LISTENER_H_
#define UTIL_PROGRESS_SUB_TASK_LISTENER_H_

#include "progresslistener.h"

class SubTaskListener final : public ProgressListener {
 public:
  SubTaskListener(ProgressListener& parentListener, size_t progress,
                  size_t maxProgress)
      : _parentListener(parentListener),
        _fullProgress(progress),
        _fullMaxProgress(maxProgress) {}

  void OnStartTask(const std::string& description) override {
    _parentListener.OnStartTask(description);
  }

  void OnProgress(size_t progress, size_t maxProgress) override {
    _parentListener.OnProgress(_fullProgress * maxProgress + progress,
                               maxProgress * _fullMaxProgress);
  }

  void OnFinish() override {}

  void OnException(std::exception& thrownException) override {
    _parentListener.OnException(thrownException);
  }

 private:
  ProgressListener& _parentListener;
  size_t _fullProgress;
  size_t _fullMaxProgress;
};

#endif
