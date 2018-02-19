#ifndef PYTHON_STRATEGY_H
#define PYTHON_STRATEGY_H

#include "../../structures/timefrequencydata.h"

#include <string>

class PythonStrategy
{
public:
	PythonStrategy();
	~PythonStrategy();
	
	void Execute(TimeFrequencyData& tfData);
	
private:
	std::string getPythonError();
	
	std::string _code;
};

#endif

