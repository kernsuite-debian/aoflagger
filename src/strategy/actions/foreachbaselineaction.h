#ifndef RFISTRATEGYFOREACHBASELINEACTION_H
#define RFISTRATEGYFOREACHBASELINEACTION_H

#include "../control/actionblock.h"
#include "../control/artifactset.h"

#include "../imagesets/imageset.h"

#include <stack>
#include <set>
#include <mutex>
#include <condition_variable>

#include "../../util/progresslistener.h"

namespace rfiStrategy {

	class ForEachBaselineAction : public ActionBlock {
		public:
			ForEachBaselineAction() :
				_baselineCount(0),
				_nextIndex(0),
				_threadCount(4),
				_selection(CrossCorrelations),
				_loopIndex(nullptr),
				_artifacts(nullptr),
				_resultSet(nullptr),
				_finishedBaselines(false),
				_progressTaskNo(nullptr),
				_progressTaskCount(nullptr),
				_exceptionOccured(false),
				_baselineProgress(0),
				_hasInitAntennae(false),
				_initPartIndex(0)
			{
			}
			virtual ~ForEachBaselineAction()
			{
			}
			virtual std::string Description() final override
			{
				return "For each baseline";
			}
			virtual void Initialize() final override
			{
			}
			virtual void Perform(ArtifactSet &artifacts, ProgressListener &progress) final override;

			enum BaselineSelection Selection() const throw() { return _selection; }
			void SetSelection(enum BaselineSelection selection) throw() { _selection = selection; }
			
			size_t ThreadCount() const throw() { return _threadCount; }
			void SetThreadCount(size_t threadCount) throw() { _threadCount = threadCount; }
			
			virtual ActionType Type() const final override { return ForEachBaselineActionType; }

			std::set<size_t> &AntennaeToSkip() { return _antennaeToSkip; }
			const std::set<size_t> &AntennaeToSkip() const { return _antennaeToSkip; }
			
			std::set<size_t> &AntennaeToInclude() { return _antennaeToInclude; }
			const std::set<size_t> &AntennaToInclude() const { return _antennaeToInclude; }
			
			std::set<size_t>& Fields() { return _fields; }
			const std::set<size_t>& Fields() const { return _fields; }
			
			std::set<size_t>& Bands() { return _bands; }
			const std::set<size_t>& Bands() const { return _bands; }
		private:
			bool IsBaselineSelected(ImageSetIndex &index);
			std::unique_ptr<class ImageSetIndex> GetNextIndex();
			static std::string memToStr(double memSize);
			
			void SetExceptionOccured();
			void SetFinishedBaselines();
			void SetProgress(ProgressListener &progress, int no, int count, const std::string& taskName, int threadId);
			size_t mathThreadCount() const
			{
				// Since IO also takes some CPU, and IO should not be
				// starved by math (because it is mostly IO limited),
				// we reserve one of the threads for that.
				//if(_threadCount > 1)
				//	return _threadCount - 1;
				//else
					return _threadCount;
			}

			size_t BaselineProgress()
			{
				std::lock_guard<std::mutex> lock(_mutex);
				return _baselineProgress;
			}
			void IncBaselineProgress()
			{
				std::lock_guard<std::mutex> lock(_mutex);
				++_baselineProgress;
			}
			
			void WaitForBufferAvailable(size_t maxSize)
			{
				std::unique_lock<std::mutex> lock(_mutex);
				while(_baselineBuffer.size() > maxSize && !_exceptionOccured)
					_dataProcessed.wait(lock);
			}
			
			std::unique_ptr<BaselineData> GetNextBaseline()
			{
				std::unique_lock<std::mutex> lock(_mutex);
				while(_baselineBuffer.size() == 0 && !_exceptionOccured && !_finishedBaselines)
					_dataAvailable.wait(lock);
				if((_finishedBaselines && _baselineBuffer.size() == 0) || _exceptionOccured)
					return nullptr;
				else
				{
					std::unique_ptr<BaselineData> next = std::move(_baselineBuffer.top());
					_baselineBuffer.pop();
					_dataProcessed.notify_one();
					return next;
				}
			}

			size_t GetBaselinesInBufferCount()
			{
				std::lock_guard<std::mutex> lock(_mutex);
				return _baselineBuffer.size();
			}
			
			struct PerformFunction : public ProgressListener
			{
				PerformFunction(ForEachBaselineAction &action, ProgressListener &progress, size_t threadIndex)
				  : _action(action), _progress(progress), _threadIndex(threadIndex)
				{
				}
				PerformFunction(const PerformFunction &source)
					: ProgressListener(source), _action(source._action), _progress(source._progress), _threadIndex(source._threadIndex)
				{
				}
				ForEachBaselineAction &_action;
				ProgressListener &_progress;
				size_t _threadIndex;
				void operator()();
				virtual void OnStartTask(const Action &action, size_t taskNo, size_t taskCount, const std::string &description, size_t weight=1) final override;
				virtual void OnEndTask(const Action &action) final override;
				virtual void OnProgress(const Action &action, size_t progres, size_t maxProgress) final override;
				virtual void OnException(const Action &action, std::exception &thrownException) final override;
			};
			
			struct ReaderFunction
			{
				explicit ReaderFunction(ForEachBaselineAction &action)
				  : _action(action)
				{
				}
				void operator()();

				ForEachBaselineAction &_action;
			};
			
			size_t _baselineCount, _nextIndex;
			size_t _threadCount;
			BaselineSelection _selection;

			std::unique_ptr<ImageSetIndex> _loopIndex;
			ArtifactSet *_artifacts, *_resultSet;
			
			std::mutex _mutex;
			std::condition_variable _dataAvailable, _dataProcessed;
			std::stack<std::unique_ptr<BaselineData>> _baselineBuffer;
			bool _finishedBaselines;

			int *_progressTaskNo, *_progressTaskCount;
			bool _exceptionOccured;
			size_t _baselineProgress;
			
			// Initial data
			AntennaInfo _initAntenna1, _initAntenna2;
			bool _hasInitAntennae;
			size_t _initPartIndex;
			std::set<size_t> _antennaeToInclude;
			std::set<size_t> _antennaeToSkip;
			std::set<size_t> _fields;
			std::set<size_t> _bands;
	};
}

#endif
