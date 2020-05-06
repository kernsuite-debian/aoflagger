#ifndef LUA_THREAD_GROUP_H
#define LUA_THREAD_GROUP_H

#include "luastrategy.h"

#include <vector>

class LuaThreadGroup
{
public:
	LuaThreadGroup(size_t nThreads) : _strategies(nThreads)
	{ }
	
	void Load(const char* filename)
	{
		for(LuaStrategy& s : _strategies)
			s.Load(filename);
	}
	void Execute(size_t threadIndex, class TimeFrequencyData& tfData, const TimeFrequencyMetaDataCPtr& metaData, class ScriptData& scriptData)
	{
		_strategies[threadIndex].Execute(tfData, metaData, scriptData);
	}
	void RunPreamble(const std::vector<std::string>& preamble)
	{
		for(LuaStrategy& s : _strategies)
			s.RunPreamble(preamble);
	}
	
	size_t NThreads() const { return _strategies.size(); }
	
	const LuaStrategy& GetThread(size_t index) const { return _strategies[index]; }
	LuaStrategy& GetThread(size_t index) { return _strategies[index]; }
	
private:
	std::vector<LuaStrategy> _strategies;
};

#endif

