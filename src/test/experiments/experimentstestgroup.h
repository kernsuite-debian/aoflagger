#ifndef AOFLAGGER_EXPERIMENTSTESTGROUP_H
#define AOFLAGGER_EXPERIMENTSTESTGROUP_H

#include "../testingtools/testgroup.h"

#include "defaultstrategyspeedtest.h"
#include "highpassfilterexperiment.h"

#include "filterresultstest.h"
#include "scaleinvariantdilationexperiment.h"
#include "rankoperatorrocexperiment.h"

class ExperimentsTestGroup : public TestGroup {
	public:
		ExperimentsTestGroup() : TestGroup("Experiments") { }
		
		virtual void Initialize() override
		{
			Add(new HighPassFilterExperiment());
			Add(new DefaultStrategySpeedTest());
			
			// Optionals:
			//Add(new RankOperatorROCExperiment());
			//Add(new FilterResultsTest());
			//Add(new ScaleInvariantDilationExperiment());
		}
};

#endif
