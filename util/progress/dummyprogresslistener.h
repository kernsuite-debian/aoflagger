#ifndef UTIL_PROGRESS_DUMMY_PROGRESS_LISTENER_H_
#define UTIL_PROGRESS_DUMMY_PROGRESS_LISTENER_H_

#include "progresslistener.h"

#include <exception>
#include <string>

class DummyProgressListener final : public ProgressListener {
  void OnStartTask(const std::string&) override {}
  void OnProgress(size_t, size_t) override {}
  void OnFinish() override {}
  void OnException(std::exception&) override {}
};

#endif
