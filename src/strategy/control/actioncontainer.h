#include "../actions/action.h"

#ifndef RFIACTIONCONTAINER_H
#define RFIACTIONCONTAINER_H 

#include <memory>
#include <vector>

namespace rfiStrategy {

	class Action;
	
	class ActionContainer : public Action
	{
		public:
			typedef std::vector<std::unique_ptr<Action>>::const_iterator const_iterator;
			typedef std::vector<std::unique_ptr<Action>>::iterator iterator;

			void Add(std::unique_ptr<Action> newAction);
			std::unique_ptr<Action> RemoveAndAcquire(class Action *action);
			void RemoveAndDelete(class Action *action);
			void RemoveAll();
			size_t GetChildCount() const throw() { return _childActions.size(); }
			Action &GetChild(size_t index) const { return *_childActions[index]; }
			Action &GetFirstChild() const { return *_childActions.front(); }
			Action &GetLastChild() const { return *_childActions.back(); }
			void MoveChildUp(size_t childIndex)
			{
				if(childIndex > 0)
				{
					std::swap(_childActions[childIndex], _childActions[childIndex-1]);
				}
			}
			void MoveChildDown(size_t childIndex)
			{
				if(childIndex < _childActions.size()-1)
				{
					std::swap(_childActions[childIndex], _childActions[childIndex+1]);
				}
			}
			void InitializeAll();
			void FinishAll();

			iterator begin() { return _childActions.begin(); }
			iterator end() { return _childActions.end(); }
			const_iterator begin() const { return _childActions.begin(); }
			const_iterator end() const { return _childActions.end(); }
		private:
			std::vector<std::unique_ptr<Action>> _childActions;
	};
}

#endif // RFIACTIONCONTAINER_H
