#ifndef PROGRESSLISTENER_H
#define PROGRESSLISTENER_H

#include <iostream>
#include <string>

class ProgressListener {
 public:
  ProgressListener() {}
  virtual ~ProgressListener() {}

  virtual void OnStartTask(const std::string& description) = 0;

  virtual void OnProgress(size_t progress, size_t maxProgress) = 0;

  /**
   * This function will be called exactly once, unless
   * an exception occurred. In that case, @ref OnException() will
   * be called only.
   */
  virtual void OnFinish() = 0;

  virtual void OnException(std::exception& thrownException) = 0;
};

class DummyProgressListener final : public ProgressListener {
  virtual void OnStartTask(const std::string&) override {}
  virtual void OnProgress(size_t, size_t) override {}
  virtual void OnFinish() override {}
  virtual void OnException(std::exception&) override {}
};

class StdOutReporter final : public ProgressListener {
 public:
  void OnStartTask(const std::string& description) override {
    std::cout << description << "...\n";
  }
  void OnProgress(size_t progress, size_t maxProgress) override {
    const unsigned twoPercent = (maxProgress + 49) / 50;
    if ((progress % twoPercent) == 0) {
      if (((progress / twoPercent) % 5) == 0)
        std::cout << (100 * progress / maxProgress) << std::flush;
      else
        std::cout << '.' << std::flush;
    }
  }
  void OnFinish() override { std::cout << "100\n"; }
  void OnException(std::exception& thrownException) override {
    std::cout << "ERROR! " << thrownException.what() << '\n';
  }
};

#endif  // PROGRESSLISTENER_H
