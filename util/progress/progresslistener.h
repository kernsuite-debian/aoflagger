#ifndef UTIL_PROGRESS_PROGRESS_LISTENER_H_
#define UTIL_PROGRESS_PROGRESS_LISTENER_H_

#include <exception>
#include <string>

class ProgressListener {
 public:
  ProgressListener() = default;
  virtual ~ProgressListener() = default;

  virtual void OnStartTask(const std::string& description) = 0;

  virtual void OnProgress(size_t progress, size_t max_progress) = 0;

  /**
   * This function will be called exactly once, unless
   * an exception occurred. In that case, @ref OnException() will
   * be called only.
   */
  virtual void OnFinish() = 0;

  virtual void OnException(std::exception& thrown_exception) = 0;
};

#endif  // UTIL_PROGRESS_PROGRESS_LISTENER_H_
