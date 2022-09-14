#ifndef UTIL_PROGRESS_STD_OUT_REPORTER_H_
#define UTIL_PROGRESS_STD_OUT_REPORTER_H_

#include "progresslistener.h"

#include <exception>
#include <string>
#include <iostream>

class StdOutReporter final : public ProgressListener {
 public:
  void OnStartTask(const std::string& description) override {
    std::cout << description << "...\n";
  }
  void OnProgress(size_t progress, size_t max_progress) override {
    const unsigned two_percent = (max_progress + 49) / 50;
    if ((progress % two_percent) == 0) {
      if (((progress / two_percent) % 5) == 0)
        std::cout << (100 * progress / max_progress) << std::flush;
      else
        std::cout << '.' << std::flush;
    }
  }
  void OnFinish() override { std::cout << "100\n"; }
  void OnException(std::exception& thrown_exception) override {
    std::cerr << "ERROR! " << thrown_exception.what() << '\n';
  }
};

#endif  // UTIL_PROGRESS_STD_OUT_REPORTER_H_
