#ifndef IMAGE2DTEST_H
#define IMAGE2DTEST_H

#include "../testingtools/asserter.h"
#include "../testingtools/unittest.h"

class Image2DTest : public UnitTest {
public:
	Image2DTest() : UnitTest("Image2D class")
	{
		AddTest(Construction(), "Construction");
		AddTest(Assignment(), "Assignment");
	}
	
	struct Construction : public Asserter
	{
		void operator()();
	};
	
	struct Assignment : public Asserter
	{
		void operator()();
	};
	
	static void AssertAll(Asserter& asserter, const Image2D& image, float value, const std::string& description = "")
	{
		for(size_t y=0; y!=image.Height(); ++y)
		{
			for(size_t x=0; x!=image.Width(); ++x)
			{
				if(description.empty())
				{
					asserter.AssertEquals(image.Value(x, y), value);
					asserter.AssertEquals(image.ValuePtr(0, y)[x], value);
				}
				else {
					asserter.AssertEquals(image.Value(x, y), value, description);
					asserter.AssertEquals(image.ValuePtr(0, y)[x], value, description);
				}
			}
		}
	}
};

void Image2DTest::Construction::operator()()
{
	Image2D image(Image2D::MakeSetImage(10, 10, 1.0));
	AssertAll(*this, image, 1.0, "Construct initialized");
	
	Image2D copy(image);
	AssertAll(*this, copy, 1.0, "Copy construct");
	
	Image2D moved(std::move(copy));
	AssertAll(*this, moved, 1.0, "Move construct");
	
	Image2D unset(Image2D::MakeUnsetImage(5, 5));
	unset.SetValue(1, 2, 3.0);
	AssertEquals(unset.Value(1, 2), 3.0, "Construct unset");
}

void Image2DTest::Assignment::operator()()
{
	Image2D image1 = Image2D::MakeSetImage(10, 10, 1.0);
	AssertAll(*this, image1, 1.0);
	
	Image2D image2 = Image2D::MakeSetImage(10, 10, 2.0);
	AssertAll(*this, image2, 2.0);
	
	image1 = image2;
	AssertAll(*this, image1, 2.0, "Assign image2 to image1");
	AssertAll(*this, image2, 2.0, "Not changed after assigned from");
	
	Image2D image3 = Image2D::MakeSetImage(10, 10, 3.0);
	AssertAll(*this, image3, 3.0);
	
	image1 = std::move(image3);
	AssertAll(*this, image1, 3.0, "Moved image3 to image1");
	
	Image2D unset(Image2D::MakeUnsetImage(5, 5));
	unset = std::move(image3);
	AssertAll(*this, image1, 3.0, "Moved image3 to unset");
}

#endif
