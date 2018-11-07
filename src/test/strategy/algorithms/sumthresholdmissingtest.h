#ifndef AOFLAGGER_SUMTHRESHOLD_MISSING_TEST_H
#define AOFLAGGER_SUMTHRESHOLD_MISSING_TEST_H

#include "../../../structures/image2d.h"
#include "../../../structures/mask2d.h"
#include "../../../structures/timefrequencydata.h"

#include "../../../strategy/algorithms/sumthresholdmissing.h"
#include "../../../strategy/algorithms/testsetgenerator.h"
#include "../../../strategy/algorithms/thresholdconfig.h"

#include "../../testingtools/asserter.h"
#include "../../testingtools/maskasserter.h"
#include "../../testingtools/unittest.h"

class SumThresholdMissingTest : public UnitTest {
public:
	SumThresholdMissingTest() : UnitTest("Sumthreshold with missing values")
	{
		AddTest(Horizontal(), "Horizontal");
	}
	
private:
	struct Horizontal : public Asserter
	{
	void operator()();
	};
};

void SumThresholdMissingTest::Horizontal::operator()()
{
	const unsigned
		width = 8,
		height = 8;
	Mask2D
		mask = Mask2D::MakeSetMask<false>(width, height),
		missing = Mask2D::MakeSetMask<false>(width, height),
		scratch = Mask2D::MakeUnsetMask(width, height);
	Image2D
		image = Image2D::MakeSetImage(width, height, 0.0);
	
	for(size_t y=0; y!=height; ++y)
	{
		image.SetValue(3, y, 1.0);
		image.SetValue(4, y, 1.0);
	}
		
	SumThresholdMissing::Horizontal(image, mask, missing, scratch, 2, 0.8);
		
	for(size_t y=0; y!=height; ++y)
	{
		std::ostringstream strA;
		strA << "Value(3, " << y << ") = true";
		AssertTrue(mask.Value(3, y), strA.str());
		
		std::ostringstream strB;
		strB << "Value(4, " << y << ") = true";
		AssertTrue(mask.Value(4, y), strB.str());
		
		std::ostringstream strE;
		strE << "Value(0, " << y << ") = false";
		AssertFalse(mask.Value(0, y), strE.str());
		
		std::ostringstream strC;
		strC << "Value(2, " << y << ") = false";
		AssertFalse(mask.Value(2, y), strC.str());
		
		std::ostringstream strD;
		strD << "Value(5, " << y << ") = false";
		AssertFalse(mask.Value(5, y), strD.str());
	}
}

#endif

