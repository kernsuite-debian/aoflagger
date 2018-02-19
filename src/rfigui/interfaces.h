#ifndef GUI_INTERFACES_H
#define GUI_INTERFACES_H

#include <sigc++/signal.h>

#include <memory>

namespace rfiStrategy
{
	class Strategy;
}

class StrategyController
{
public:
	virtual void SetStrategy(std::unique_ptr<rfiStrategy::Strategy> strategy) = 0;
	virtual rfiStrategy::Strategy &Strategy() = 0;
	virtual void NotifyChange() { _signalOnStrategyChanged(); }
	virtual sigc::signal<void> SignalOnStrategyChanged() { return _signalOnStrategyChanged; }
private:
	sigc::signal<void> _signalOnStrategyChanged;
};

#endif
