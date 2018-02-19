#ifndef RFISTRATEGYACTIONFACTORY_H
#define RFISTRATEGYACTIONFACTORY_H

#include <string>
#include <vector>

#include "../actions/action.h"

namespace rfiStrategy {

	class ActionFactory {
		public:
			static const std::vector<std::string> GetActionList();
			static std::unique_ptr<class Action> CreateAction(const std::string &action);
			static const char *GetDescription(ActionType actionType);
		private:
			template<typename T> static std::unique_ptr<T> make()
			{
				return std::unique_ptr<T>(new T());
			}
			
			ActionFactory() = delete;
			~ActionFactory() = delete;
	};
}

#endif // RFISTRATEGYACTIONFACTORY_H
