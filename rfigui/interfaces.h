#ifndef GUI_INTERFACES_H
#define GUI_INTERFACES_H

#include <sigc++/signal.h>

#include <memory>

namespace imagesets {
class Strategy;
}

class StrategyController {
 public:
  virtual ~StrategyController() = default;
  virtual void SetStrategy(std::unique_ptr<imagesets::Strategy> strategy) = 0;
  virtual imagesets::Strategy& Strategy() = 0;
  virtual void NotifyChange() { _signalOnStrategyChanged(); }
  virtual sigc::signal<void> SignalOnStrategyChanged() {
    return _signalOnStrategyChanged;
  }

 private:
  sigc::signal<void> _signalOnStrategyChanged;
};

#endif
