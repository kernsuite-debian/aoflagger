#include "actioncontainer.h"

#include "../actions/action.h"

#include "strategyiterator.h"

namespace rfiStrategy {

	void ActionContainer::RemoveAll()
	{
		_childActions.clear();
	}

	void ActionContainer::Add(std::unique_ptr<Action> newAction)
	{
		newAction->_parent = this;
		_childActions.emplace_back(std::move(newAction));
	}

	std::unique_ptr<Action> ActionContainer::RemoveAndAcquire(class Action *action)
	{
		std::unique_ptr<Action> a;
		for(iterator i=_childActions.begin();i!=_childActions.end();++i)
		{
			if(i->get() == action) {
				a = std::move(*i);
				_childActions.erase(i);
				break;
			}
		}
		return a;
	}

	void ActionContainer::RemoveAndDelete(class Action *action)
	{
		for(iterator i=_childActions.begin();i!=_childActions.end();++i)
		{
			if(i->get() == action) {
				_childActions.erase(i);
				break;
			}
		}
	}

	void ActionContainer::InitializeAll()
	{
		StrategyIterator i = StrategyIterator::NewStartIterator(*this);
		while(!i.PastEnd())
		{
			i->Initialize();
			++i;
		}
	}
	
	void ActionContainer::FinishAll()
	{
		StrategyIterator i = StrategyIterator::NewStartIterator(*this);
		while(!i.PastEnd())
		{
			i->Finish();
			++i;
		}
	}
}
