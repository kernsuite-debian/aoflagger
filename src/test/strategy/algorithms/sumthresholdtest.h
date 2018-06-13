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
			AddTest(StabilitySSE(), "SumThreshold stability (SSE)");
#endif
#ifdef __AVX2__
			AddTest(SimpleVerticalSumThresholdAVX(), "Simple SumThreshold AVX case (vertical)");
			AddTest(VerticalSumThresholdAVX(), "SumThreshold optimized AVX version (vertical)");
			AddTest(StabilityAVX(), "SumThreshold stability (AVX)");
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
		struct StabilitySSE : public Asserter
		{
			void operator()();
		};
#endif
#ifdef __AVX2__
		struct SimpleVerticalSumThresholdAVX : public Asserter
		{
			void operator()();
		};
		struct VerticalSumThresholdAVX : public Asserter
		{
			void operator()();
		};
		struct StabilityAVX : public Asserter
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
	Mask2D
		mask1 = Mask2D::MakeUnsetMask(width, height),
		mask2 = Mask2D::MakeUnsetMask(width, height),
		scratch = Mask2D::MakeUnsetMask(width, height);
	Image2DPtr
		real = Image2D::MakePtr(TestSetGenerator::MakeTestSet(26, mask1, width, height)),
		imag = Image2D::MakePtr(TestSetGenerator::MakeTestSet(26, mask2, width, height));
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
		
		CombinatorialThresholder::VerticalSumThresholdLargeReference(image.get(), &mask1, &scratch, length, threshold);
		CombinatorialThresholder::VerticalSumThresholdLargeSSE(image.get(), &mask2, &scratch, length, threshold);
		
		if(length != 32) {
		  std::stringstream s;
		  s << "Equal SSE and reference masks produced by SumThreshold length " << length;
		  AssertTrue(mask1 == mask2, s.str());
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
		mask2 = Mask2D::MakeUnsetMask(width, height),
		scratch = Mask2D::MakeUnsetMask(width, height);
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
		
		CombinatorialThresholder::HorizontalSumThresholdLargeReference(image.get(), &mask1, &scratch, length, threshold);
		CombinatorialThresholder::HorizontalSumThresholdLargeSSE(image.get(), &mask2, &scratch, length, threshold);
		
		std::stringstream s;
		s << "Equal SSE and reference masks produced by SumThreshold length " << length << ", threshold " << threshold;
		MaskAsserter::AssertEqualMasks(mask2, mask1, s.str());
	}
}

void SumThresholdTest::StabilitySSE::operator()()
{
	Mask2D
		maskA = Mask2D::MakeSetMask<false>(1, 1),
		maskB = Mask2D::MakeSetMask<false>(2, 2),
		maskC = Mask2D::MakeSetMask<false>(3, 3),
		maskD = Mask2D::MakeSetMask<false>(4, 4),
		scratch = Mask2D::MakeUnsetMask(4, 4);
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
		CombinatorialThresholder::HorizontalSumThresholdLargeSSE(&realA, &maskA, &scratch, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeSSE(&realA, &maskA, &scratch, length, 1.0);
		CombinatorialThresholder::HorizontalSumThresholdLargeSSE(&realA, &maskB, &scratch, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeSSE(&realA, &maskB, &scratch, length, 1.0);
		CombinatorialThresholder::HorizontalSumThresholdLargeSSE(&realA, &maskC, &scratch, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeSSE(&realA, &maskC, &scratch, length, 1.0);
		CombinatorialThresholder::HorizontalSumThresholdLargeSSE(&realA, &maskD, &scratch, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeSSE(&realA, &maskD, &scratch, length, 1.0);
	}
}
#endif // __SSE__

#ifdef __AVX2__
void SumThresholdTest::VerticalSumThresholdAVX::operator()()
{
	const unsigned
		width = 2048,
		height = 256;
	Mask2D
		mask1 = Mask2D::MakeUnsetMask(width, height),
		mask2 = Mask2D::MakeUnsetMask(width, height),
		scratch = Mask2D::MakeUnsetMask(width, height);
	Image2DPtr
		real = Image2D::MakePtr(TestSetGenerator::MakeTestSet(26, mask1, width, height)),
		imag = Image2D::MakePtr(TestSetGenerator::MakeTestSet(26, mask2, width, height));
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
		
		CombinatorialThresholder::VerticalSumThresholdLargeReference(image.get(), &mask1, &scratch, length, threshold);
		CombinatorialThresholder::VerticalSumThresholdLargeAVX(image.get(), &mask2, &scratch, length, threshold);
		
		if(length != 32) {
		  std::stringstream s;
		  s << "Equal AVX and reference masks produced by SumThreshold length " << length;
		  AssertTrue(mask1 == mask2, s.str());
		}
	}
}

void SumThresholdTest::SimpleVerticalSumThresholdAVX::operator()()
{
	const unsigned
		width = 8,
		height = 8;
	Mask2D
		mask = Mask2D::MakeSetMask<false>(width, height),
		scratch = Mask2D::MakeUnsetMask(width, height);
	Image2D
		image = Image2D::MakeSetImage(width, height, 0.0);
	
	for(size_t x=0; x!=width; ++x)
	{
		image.SetValue(x, 3, 1.0);
		image.SetValue(x, 4, 1.0);
	}
		
	CombinatorialThresholder::VerticalSumThresholdLargeAVX(&image, &mask, &scratch, 2, 0.8);
		
	for(size_t x=0; x!=width; ++x)
	{
		std::ostringstream strA;
		strA << "Value(" << x << ", 3) = true";
		AssertTrue(mask.Value(x, 3), strA.str());
		
		std::ostringstream strB;
		strB << "Value(" << x << ", 4) = true";
		AssertTrue(mask.Value(x, 4), strB.str());
		
		std::ostringstream strE;
		strE << "Value(" << x << ", 0) = false";
		AssertFalse(mask.Value(x, 0), strE.str());
		
		std::ostringstream strC;
		strC << "Value(" << x << ", 2) = false";
		AssertFalse(mask.Value(x, 2), strC.str());
		
		std::ostringstream strD;
		strD << "Value(" << x << ", 5) = false";
		AssertFalse(mask.Value(x, 5), strD.str());
	}
}

void SumThresholdTest::StabilityAVX::operator()()
{
	Mask2D
		maskA = Mask2D::MakeSetMask<false>(1, 1),
		maskB = Mask2D::MakeSetMask<false>(2, 2),
		maskC = Mask2D::MakeSetMask<false>(3, 3),
		maskD = Mask2D::MakeSetMask<false>(4, 4),
		scratch = Mask2D::MakeUnsetMask(4, 4);
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
		//CombinatorialThresholder::HorizontalSumThresholdLargeAVX(&realA, &maskA, &scratch, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeAVX(&realA, &maskA, &scratch, length, 1.0);
		//CombinatorialThresholder::HorizontalSumThresholdLargeAVX(&realA, &maskB, &scratch, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeAVX(&realA, &maskB, &scratch, length, 1.0);
		//CombinatorialThresholder::HorizontalSumThresholdLargeAVX(&realA, &maskC, &scratch, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeAVX(&realA, &maskC, &scratch, length, 1.0);
		//CombinatorialThresholder::HorizontalSumThresholdLargeAVX(&realA, &maskD, &scratch, length, 1.0);
		CombinatorialThresholder::VerticalSumThresholdLargeAVX(&realA, &maskD, &scratch, length, 1.0);
	}
}
#endif // __AVX2__

#endif
