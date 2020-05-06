#ifndef RUNNER_H
#define RUNNER_H

#include "options.h"

#include "../strategy/imagesets/imageset.h"

class Runner
{
public:
	Runner(const Options& options) :
		_options(options)
	{ }
	
	void Run();
	
private:
	void updateOptions(class LuaStrategy& lua);
	void processFile(const std::string& filename, class LuaThreadGroup& lua);
	void writeHistory(const std::string &filename);
	Options _options;
};

#endif
