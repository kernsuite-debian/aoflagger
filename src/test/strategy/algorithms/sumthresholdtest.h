#ifndef AOFLAGGER_SUMTHRESHOLDTEST_H
#define AOFLAGGER_SUMTHRESHOLDTEST_H

#include "../../../structures/image2d.h"
#include "../../../structures/mask2d.h"
#include "../../../structures/timefrequencydata.h"

#include "../../../strategy/algorithms/combinatorialthresholder.h"
#include "../../../strategy/algorithms/testsetgenerator.h"
#include "../../../strategy/algorithms/thresholdconfig.h"

#include "../../testingtools/asserter.h"
#include "../../testingtools/maskasserter.h"
#include "../../testingtools/unittest.h"

class SumThresholdTest : public UnitTest {
	public:
		SumThresholdTest() : UnitTest("Sumthreshold")
		{
#ifdef __SSE__
			AddTest(VerticalSumThresholdSSE(), "SumThreshold optimized SSE version (vertical)");
			AddTest(HorizontalSumThresholdSSE(), "SumThreshold optimized SSE version (horizontal)");
			AddTest(Stability(), "SumThreshold stability");
#endif
		}
		
	private:
#ifdef __SSE__
		struct VerticalSumThresholdSSE : public Asserter
		{
			void operator()();
		};
		struct HorizontalSumThresholdSSE : public Asserter
		{
			void operator()();
		};
		struct Stability : public Asserter
		{
			void operator()();
		};
#endif
};

#ifdef __SSE__
void SumThresholdTest::VerticalSumThresholdSSE::operator()()
{
	const unsigned
		width = 2048,
		height = 256;
	Mask2DPtr
		mask1 = Mask2D::CreateUnsetMaskPtr(width, height),
		mask2 = Mask2D::CreateUnsetMaskPtr(width, height);
	Image2DPtr
		real = Image2D::MakePtr(TestSetGenerator::MakeTestSet(26, *mask1, width, height)),
		imag = Image2D::MakePtr(TestSetGenerator::MakeTestSet(26, *mask2, width, height));
	TimeFrequencyData data(Polarization::XX, real, imag);
	Image2DCPtr image = data.GetSingleImage();
	
	ThresholdConfig config;
	config.InitializeLengthsDefault(9);
	num_t mode = image->GetMode();
	config.InitializeThresholdsFromFirstThreshold(6.0 * mode, ThresholdConfig::Rayleigh);
	for(unsigned i=0;i<9;++i)
	{
		mask1->SetAll<false>();
		mask2->SetAll<false>();
		
		const unsigned length = config.GetHorizontalLength(i);
		const double threshold = config.GetHorizontalThreshold(i);
		
		CombinatorialThresholder::VerticalSumThresholdLargeReference(image.get(), mask1.get(), length, threshold);
		CombinatorialThresholder::VerticalSumThresholdLargeSSE(image.get(), mask2.get(), length, threshold);
		
		if(length != 32) {
		  std::stringstream s;
		  s << "Equal SSE and reference masks produced by SumThreshold length " << length;
		  AssertTrue(mask1->Equals(mask2), s.str());
		}
	}
}

void SumThresholdTest::HorizontalSumThresholdSSE::operator()()
{
	const unsigned
		width = 2048,
		height = 256;
	Mask2D
		mask1 = Mask2D::MakeUnsetMask(width, height),
		mask2 = Mask2D::MakeUnsetMask(width, height);
	Image2DPtr
		real = Image2D::MakePtr(TestSetGenerator::MakeTestSet(26, mask1, width, height)),
		imag = Image2D::MakePtr(TestSetGenerator::MakeTestSet(26, mask2, width, height));
		
	mask1.SwapXY();
	mask2.SwapXY();
	real->SwapXY();
	imag->SwapXY();
		
	TimeFrequencyData data(Polarization::XX, real, imag);
	Image2DCPtr image = data.GetSingleImage();

	ThresholdConfig config;
	config.InitializeLengthsDefault(9);
	num_t mode = image->GetMode();
	config.InitializeThresholdsFromFirstThreshold(6.0 * mode, ThresholdConfig::Rayleigh);
	for(unsigned i=0;i<9;++i)
	{
		mask1.SetAll<false>();
		mask2.SetAll<false>();
		
		const unsigned length = config.GetHorizontalLength(i);
		const double threshold = config.GetHorizontalThreshold(i);
		
		CombinatorialThresholder::HorizontalSumThresholdLargeReference(image.get(), &mask1, length, threshold);
		CombinatorialThresholder::HorizontalSumThresholdLargeSSE(image.get(), &mask2, length, threshold);
		
		std::stringstream s;
		s << "Equal SSE and reference masks produced by SumThreshold length " << length << ", threshold " << threshold;
		MaskAsserter::AssertEqualMasks(mask2, mask1, s.str());
	}
}

void SumThresholdTest::Stability::operator()()
{
	Mask2D
		maskA = Mask2D::MakeSetMask<false>(1, 1),
		maskB = Mask2D::MakeSetMask<false>(2, 2),
		maskC = Mask2D::MakeSetMask<false>(3, 3),
		maskD = Mask2D::MakeSetMask<false>(4, 4);
	Image2D
		realA = Image2D::MakeZeroImage(1, 1),
		realB = Image2D::MakeZeroImage(2, 2),
		realC = Image2D::MakeZeroImage(3, 3),
		realD = Image2D::MakeZeroImage(4, 4);
		
	ThresholdConfig config;
	config.InitializeLengthsDefault(9);
	config.InitializeThresholdsFromFirstThreshold(6.0, ThresholdConfig::Rayleigh);
	for(unsigned i=0;i<9;++i)
	{
		const unsigned length = config.GetHorizontalLength(i);
		CombinatorialThresholder::HorizontalSumThresholdLargeSSE(&realA, &maskA, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeSSE(&realA, &maskA, length, 1.0);
		CombinatorialThresholder::HorizontalSumThresholdLargeSSE(&realA, &maskB, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeSSE(&realA, &maskB, length, 1.0);
		CombinatorialThresholder::HorizontalSumThresholdLargeSSE(&realA, &maskC, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeSSE(&realA, &maskC, length, 1.0);
		CombinatorialThresholder::HorizontalSumThresholdLargeSSE(&realA, &maskD, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeSSE(&realA, &maskD, length, 1.0);
	}
}
#endif // __SSE__

#endif
