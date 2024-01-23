#ifndef LUA_THREAD_GROUP_H
#define LUA_THREAD_GROUP_H

#include "luastrategy.h"

#include <vector>

class LuaThreadGroup {
 public:
  explicit LuaThreadGroup(size_t nThreads) : _strategies(nThreads) {}

  void LoadFile(const char* filename) {
    for (LuaStrategy& s : _strategies) {
      s.Initialize();
      s.LoadFile(filename);
    }
  }
  void LoadText(const std::string& text) {
    for (LuaStrategy& s : _strategies) {
      s.Initialize();
      s.LoadText(text);
    }
  }
  void Execute(size_t threadIndex, class TimeFrequencyData& tfData,
               const TimeFrequencyMetaDataCPtr& metaData,
               class ScriptData& scriptData,
               const std::string& executeFunctionName) {
    _strategies[threadIndex].Execute(tfData, metaData, scriptData,
                                     executeFunctionName);
  }
  void RunPreamble(const std::vector<std::string>& preamble) {
    for (LuaStrategy& s : _strategies) s.RunPreamble(preamble);
  }

  size_t NThreads() const { return _strategies.size(); }

  const LuaStrategy& GetThread(size_t index) const {
    return _strategies[index];
  }
  LuaStrategy& GetThread(size_t index) { return _strategies[index]; }

 private:
  std::vector<LuaStrategy> _strategies;
};

#endif
