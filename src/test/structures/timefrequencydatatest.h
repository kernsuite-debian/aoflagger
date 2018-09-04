#ifndef TIME_FREQUENCY_DATA_TEST_H
#define TIME_FREQUENCY_DATA_TEST_H

#include "../testingtools/asserter.h"
#include "../testingtools/unittest.h"

#include "../../structures/timefrequencydata.h"

class TimeFrequencyDataTest : public UnitTest {
public:
	TimeFrequencyDataTest() : UnitTest("TimeFrequencyData class")
	{
		AddTest(Construction(), "Construction");
		AddTest(Assignment(), "Assignment");
		AddTest(Conversion(), "Conversion");
	}
	
	struct Construction : public Asserter
	{
		void operator()();
	};
	
	struct Assignment : public Asserter
	{
		void operator()();
	};
	
	struct Conversion : public Asserter
	{
		void operator()();
	};
};

inline void TimeFrequencyDataTest::Construction::operator()()
{
	TimeFrequencyData a;
	AssertEquals(a.PolarizationCount(), size_t(0), "polarization count");
	
	Image2DPtr image = Image2D::CreateSetImagePtr(10, 10, 1.0);
	TimeFrequencyData b = TimeFrequencyData::FromLinear(
		image, image, image, image, image, image, image, image);
	AssertEquals(b.PolarizationCount(), size_t(4), "polarization count");
	AssertEquals(b.ComplexRepresentation(), TimeFrequencyData::ComplexParts, "complex representation");
	
	TimeFrequencyData c = TimeFrequencyData(
		Polarization::XX, image, image,
		Polarization::YY, image, image);
	AssertEquals(c.PolarizationCount(), size_t(2), "polarization count");
	AssertEquals(c.ComplexRepresentation(), TimeFrequencyData::ComplexParts, "complex representation");
	
	TimeFrequencyData d = TimeFrequencyData(TimeFrequencyData::AmplitudePart,
		Polarization::XX, image, 
		Polarization::YY, image);
	AssertEquals(d.PolarizationCount(), size_t(2), "polarization count");
	AssertEquals(d.ComplexRepresentation(), TimeFrequencyData::AmplitudePart, "complex representation");
	
	TimeFrequencyData e = TimeFrequencyData(
		Polarization::RR, image, image,
		Polarization::LL, image, image);
	AssertEquals(c.PolarizationCount(), size_t(2), "polarization count");
	AssertEquals(c.ComplexRepresentation(), TimeFrequencyData::ComplexParts, "complex representation");
	
	TimeFrequencyData f = TimeFrequencyData(TimeFrequencyData::AmplitudePart,
		Polarization::RR, image, 
		Polarization::LL, image);
	AssertEquals(d.PolarizationCount(), size_t(2), "polarization count");
	AssertEquals(d.ComplexRepresentation(), TimeFrequencyData::AmplitudePart, "complex representation");
}

inline void TimeFrequencyDataTest::Assignment::operator()()
{
	
}

inline void TimeFrequencyDataTest::Conversion::operator()()
{
	Image2DPtr image = Image2D::CreateSetImagePtr(10, 10, 1.0);
	TimeFrequencyData data = TimeFrequencyData(
		Polarization::XX, image, image,
		Polarization::YY, image, image);
	TimeFrequencyData stokesI = data.Make(Polarization::StokesI);
	AssertEquals(stokesI.PolarizationCount(), size_t(1), "I from XX/YY polarization count");
	AssertEquals(stokesI.ComplexRepresentation(), TimeFrequencyData::ComplexParts, "complex representation");
	// Test whether this does not crash
	Image2DCPtr derivedImage = stokesI.GetSingleImage();
	
	data = TimeFrequencyData(TimeFrequencyData::AmplitudePart,
		Polarization::XX, image,
		Polarization::YY, image);
	stokesI = data.Make(Polarization::StokesI);
	AssertEquals(stokesI.PolarizationCount(), size_t(1), "I from XX/YY ampl polarization count");
	AssertEquals(stokesI.ComplexRepresentation(), TimeFrequencyData::AmplitudePart, "complex representation");
	// Test whether this does not crash
	derivedImage = stokesI.GetSingleImage();
	
	data = TimeFrequencyData(
		Polarization::RR, image, image,
		Polarization::LL, image, image);
	stokesI = data.Make(Polarization::StokesI);
	AssertEquals(stokesI.PolarizationCount(), size_t(1), "I from RR/LL polarization count");
	AssertEquals(stokesI.ComplexRepresentation(), TimeFrequencyData::ComplexParts, "complex representation");
	// Test whether this does not crash
	derivedImage = stokesI.GetSingleImage();
	
	data = TimeFrequencyData(TimeFrequencyData::AmplitudePart,
		Polarization::RR, image,
		Polarization::LL, image);
	stokesI = data.Make(Polarization::StokesI);
	AssertEquals(stokesI.PolarizationCount(), size_t(1), "I from RR/LL amp polarization count");
	AssertEquals(stokesI.ComplexRepresentation(), TimeFrequencyData::AmplitudePart, "complex representation");
	// Test whether this does not crash
	derivedImage = stokesI.GetSingleImage();
	
	TimeFrequencyData xx(TimeFrequencyData::AmplitudePart, Polarization::XX, image);
	TimeFrequencyData yy(TimeFrequencyData::AmplitudePart, Polarization::YY, image);
	data = TimeFrequencyData::MakeFromPolarizationCombination(xx, yy);
	AssertEquals(data.PolarizationCount(), size_t(2), "polarization count");
	AssertEquals(data.ComplexRepresentation(), TimeFrequencyData::AmplitudePart, "complex representation");
	
	xx = TimeFrequencyData(Polarization::XX, image, image);
	yy = TimeFrequencyData(Polarization::YY, image, image);
	data = TimeFrequencyData::MakeFromPolarizationCombination(xx, yy);
	AssertEquals(data.PolarizationCount(), size_t(2), "polarization count");
	AssertEquals(data.ComplexRepresentation(), TimeFrequencyData::ComplexParts, "complex representation");
	
	TimeFrequencyData rr(TimeFrequencyData::AmplitudePart, Polarization::XX, image);
	TimeFrequencyData rl(TimeFrequencyData::AmplitudePart, Polarization::YY, image);
	TimeFrequencyData lr(TimeFrequencyData::AmplitudePart, Polarization::XX, image);
	TimeFrequencyData ll(TimeFrequencyData::AmplitudePart, Polarization::YY, image);
	data = TimeFrequencyData::MakeFromPolarizationCombination(
		TimeFrequencyData::MakeFromPolarizationCombination(rr, ll),
		TimeFrequencyData::MakeFromPolarizationCombination(rl, lr));
	AssertEquals(data.PolarizationCount(), size_t(4), "polarization count");
	AssertEquals(data.ComplexRepresentation(), TimeFrequencyData::AmplitudePart, "complex representation");
	stokesI = data.Make(Polarization::StokesI);
	AssertEquals(stokesI.PolarizationCount(), size_t(1), "I from circular amp polarization count");
	AssertEquals(stokesI.ComplexRepresentation(), TimeFrequencyData::AmplitudePart, "complex representation");
}

#endif

