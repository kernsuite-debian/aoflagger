#ifndef RFISTRATEGY_H
#define RFISTRATEGY_H 

#include <string>
#include <vector>
#include <thread>

#include "../../structures/timefrequencydata.h"

#include "action.h"
#include "foreachbaselineaction.h"

#include "../control/actionblock.h"
#include "../control/actionfactory.h"

namespace rfiStrategy {

	class Strategy : public ActionBlock
	{
		public:
			Strategy() noexcept :
				_threadFunc(nullptr),
				_thread(nullptr)
			{
				
			}
			virtual ~Strategy() {
				ArtifactSet* artifacts = JoinThread();
				delete artifacts;
			}

			virtual std::string Description() final override { return "Strategy"; }

			static void SetThreadCount(ActionContainer &strategy, size_t threadCount);
			static void SetDataColumnName(Strategy &strategy, const std::string &dataColumnName);
			
			void StartPerformThread(const class ArtifactSet &artifacts, class ProgressListener &progress);
			ArtifactSet *JoinThread();

			static void SyncAll(ActionContainer &root);

			virtual void Perform(class ArtifactSet &artifacts, class ProgressListener &listener) final override
			{
				listener.OnStartTask(*this, 0, 1, "strategy");
				try {
					ActionBlock::Perform(artifacts, listener);
				} catch(std::exception &e)
				{
					listener.OnException(*this, e);
				}
				listener.OnEndTask(*this);
			}
			virtual ActionType Type() const final override { return StrategyType; }
		protected:
		private:
			/** Copying prohibited */
			Strategy(const Strategy &) = delete;
			Strategy &operator=(const Strategy &) = delete;
			
			struct PerformFunc {
				PerformFunc(class Strategy *strategy, class ArtifactSet *artifacts, class ProgressListener *progress)
				: _strategy(strategy), _artifacts(artifacts), _progress(progress)
				{
				}
				class Strategy *_strategy;
				class ArtifactSet *_artifacts;
				class ProgressListener *_progress;

				void operator()();
			} *_threadFunc;

			std::thread *_thread;
	};
}

#endif // RFISTRATEGY_H
