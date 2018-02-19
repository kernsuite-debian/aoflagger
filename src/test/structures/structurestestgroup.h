#ifndef AOFLAGGER_STRUCTURESTESTGROUP_H
#define AOFLAGGER_STRUCTURESTESTGROUP_H

#include "../testingtools/testgroup.h"

#include "image2dtest.h"

class StructuresTestGroup : public TestGroup {
	public:
		StructuresTestGroup() : TestGroup("Generic structures") { }
		
		virtual void Initialize() override
		{
			Add(new Image2DTest());
		}
};

#endif

