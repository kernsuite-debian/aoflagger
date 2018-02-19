#include "image2d.h"

#include "../msio/fitsfile.h"

#include "../util/uvector.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <cstring>

#include <boost/numeric/conversion/bounds.hpp>

#ifdef __SSE__
#define USE_INTRINSICS
#endif

#ifdef USE_INTRINSICS
#include <xmmintrin.h>
#endif

Image2D::Image2D() noexcept :
	_width(0),
	_height(0),
	_stride(0),
	_dataPtr(nullptr),
	_dataConsecutive(nullptr)
{
}

Image2D::Image2D(size_t width, size_t height, size_t widthCapacity) :
	_width(width),
	_height(height),
	_stride((((widthCapacity-1)/4)+1)*4)
{
	if(widthCapacity == 0) _stride=0;
	allocate();
}

void Image2D::allocate()
{
	unsigned allocHeight = ((((_height-1)/4)+1)*4);
	if(_height == 0) allocHeight = 0;
#ifdef __APPLE__
		// OS-X has no posix_memalign, but malloc always uses 16-byte alignment.
		_dataConsecutive = (num_t*)malloc(_stride * allocHeight * sizeof(num_t));
#else
		if(posix_memalign((void **) &_dataConsecutive, 16, _stride * allocHeight * sizeof(num_t)) != 0)
			throw std::bad_alloc();
#endif
	_dataPtr = new num_t*[allocHeight];
	for(size_t y=0;y<_height;++y)
	{
		_dataPtr[y] = &_dataConsecutive[_stride * y];
		// Even though the values after the requested width are never relevant, we will
		// initialize them to zero to prevent valgrind to report unset values when they
		// are used in SSE instructions.
		for(size_t x=_width;x<_stride;++x)
		{
			_dataPtr[y][x] = 0.0;
		}
	}
	for(size_t y=_height;y<allocHeight;++y)
	{
		_dataPtr[y] = &_dataConsecutive[_stride * y];
		// (see remark above about initializing to zero)
		for(size_t x=0;x<_stride;++x)
		{
			_dataPtr[y][x] = 0.0;
		}
	}
}

Image2D::Image2D(const Image2D& source) :
	_width(source._width),
	_height(source._height),
	_stride(source._stride)
{
	allocate();
	std::copy(source._dataConsecutive, source._dataConsecutive+_stride * _height, _dataConsecutive);
}

Image2D::Image2D(Image2D&& source) noexcept :
	_width(source._width),
	_height(source._height),
	_stride(source._stride),
	_dataPtr(source._dataPtr),
	_dataConsecutive(source._dataConsecutive)
{
	source._width = 0;
	source._stride = 0;
	source._height = 0;
	source._dataPtr = nullptr;
	source._dataConsecutive = nullptr;
}

Image2D::~Image2D() noexcept
{
	delete[] _dataPtr;
	free(_dataConsecutive);
}

Image2D& Image2D::operator=(const Image2D& rhs)
{
	if(_width != rhs._width ||
		_height != rhs._height ||
		_stride != rhs._stride)
	{
		delete[] _dataPtr;
		free(_dataConsecutive);
		_width = rhs._width;
		_height = rhs._height;
		_stride = rhs._stride;
		allocate();
	}
	std::copy(
		rhs._dataConsecutive,
		rhs._dataConsecutive + _stride * _height,
		_dataConsecutive);
	return *this;
}

Image2D& Image2D::operator=(Image2D&& rhs) noexcept
{
	std::swap(rhs._width, _width);
	std::swap(rhs._stride, _stride);
	std::swap(rhs._height, _height);
	std::swap(rhs._dataPtr, _dataPtr);
	std::swap(rhs._dataConsecutive, _dataConsecutive);
	return *this;
}

Image2D *Image2D::CreateSetImage(size_t width, size_t height, num_t initialValue) 
{
	Image2D *image = new Image2D(width, height);
	image->SetAll(initialValue);
	return image;
}

Image2D *Image2D::CreateSetImage(size_t width, size_t height, num_t initialValue, size_t widthCapacity) 
{
	Image2D *image = new Image2D(width, height, widthCapacity);
	image->SetAll(initialValue);
	return image;
}

Image2D Image2D::MakeFromSum(const Image2D &imageA, const Image2D &imageB)
{
	if(imageA.Width() != imageB.Width() || imageA.Height() != imageB.Height())
		throw BadUsageException("Images do not match in size");
	Image2D image(imageA.Width(), imageA.Height());
	const size_t total = imageA._stride * imageA.Height();
	for(size_t i=0;i<total;++i) {
		image._dataConsecutive[i] = imageA._dataConsecutive[i] + imageB._dataConsecutive[i];
	}
	return image;
}

Image2D Image2D::MakeFromDiff(const Image2D &imageA, const Image2D &imageB)
{
	if(imageA.Width() != imageB.Width() || imageA.Height() != imageB.Height())
		throw BadUsageException("Images do not match in size");
	Image2D image(imageA.Width(), imageA.Height());
	const float *lhsPtr = &(imageA._dataConsecutive[0]);
	const float *rhsPtr = &(imageB._dataConsecutive[0]);
	float *destPtr = &(image._dataConsecutive[0]);
	const float *end = lhsPtr + imageA._stride * imageA._height;
	while(lhsPtr < end)
	{
#ifdef USE_INTRINSICS
		_mm_store_ps(destPtr, _mm_sub_ps(_mm_load_ps(lhsPtr), _mm_load_ps(rhsPtr)));
		lhsPtr += 4;
		rhsPtr += 4;
		destPtr += 4;
#else
		(*destPtr) = (*lhsPtr) - (*rhsPtr);
		lhsPtr++;
		rhsPtr++;
		destPtr++;
#endif
	}
	return image;
}

void Image2D::SetAll(num_t value)
{
	float* ptr = &_dataConsecutive[0];
	float *end = ptr + _stride * _height;
	std::fill(ptr, end, value);
}

num_t Image2D::GetAverage() const {
	size_t count = 0;
	num_t total = 0.0;
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x) {
			total += _dataPtr[y][x];
			count++;
		}
	}
	return total/(num_t) count;
}

num_t Image2D::GetMaximum() const {
	num_t max = _dataPtr[0][0];
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x) {
			if(_dataPtr[y][x] > max) {
				max = _dataPtr[y][x];
			}
		}
	}
	return max;
}

num_t Image2D::GetMinimum() const {
	num_t min = _dataPtr[0][0];
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x) {
			if(_dataPtr[y][x] < min) {
				min = _dataPtr[y][x];
			}
		}
	}
	return min;
}

num_t Image2D::GetMaximumFinite() const {
	num_t max = boost::numeric::bounds<double>::lowest();
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x) {
			if(std::isfinite(_dataPtr[y][x]) && _dataPtr[y][x] > max) {
				max = _dataPtr[y][x];
			}
		}
	}
	return max;
}

num_t Image2D::GetMinimumFinite() const {
	num_t min = std::numeric_limits<num_t>::max();
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x) {
			if(std::isfinite(_dataPtr[y][x]) && _dataPtr[y][x] < min) {
				min = _dataPtr[y][x];
			}
		}
	}
	return min;
}

bool Image2D::ContainsOnlyZeros() const 
{
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x)
		{
			if(_dataPtr[y][x] != 0.0)
				return false;
		}
	}
	return true;
}

num_t Image2D::GetMaxMinNormalizationFactor() const
{
	num_t max = GetMaximum(), min = GetMinimum();
	num_t range = (-min) > max ? (-min) : max;
	return 1.0/range;
}

num_t Image2D::GetStdDev() const
{
	num_t mean = GetAverage();
	size_t count = 0;
	num_t total = 0.0;
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x)
		{
			total += (_dataPtr[y][x]-mean)*(_dataPtr[y][x]-mean);
			count++;
		}
	}
	return sqrt(total / (num_t) count);
}

num_t Image2D::GetMode() const
{
	size_t size = _width * _height;

	num_t mode = 0.0;
	for(size_t y = 0;y<_height;++y) {
		for(size_t x = 0;x<_width; ++x) {
			const num_t value = _dataPtr[y][x];
			mode += value * value;
		}
	}
	return sqrtn(mode / (2.0L * (num_t) size));
}

num_t Image2D::GetRMS(size_t xOffset, size_t yOffset, size_t width, size_t height) const
{
	size_t count = 0;
	num_t total = 0.0;
	for(size_t y=yOffset;y<height+yOffset;++y)
	{
		for(size_t x=xOffset;x<width+xOffset;++x)
		{
			num_t v = Value(x, y);
			total += v * v;
			count++;
		}
	}
	return sqrtn(total / (num_t) count);
}

void Image2D::NormalizeVariance()
{
	num_t variance = GetStdDev();
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x)
		{
			_dataPtr[y][x] /= variance;
		}
	}
}

Image2D Image2D::MakeFromFits(FitsFile &file, int imageNumber)
{
	int dimensions = file.GetCurrentImageDimensionCount();
	if(dimensions >= 2) {
		Image2D image(file.GetCurrentImageSize(1), file.GetCurrentImageSize(2));
		ao::uvector<num_t> buffer(image._width * image._height);
		file.ReadCurrentImageData(buffer.size()*imageNumber, buffer.data(), buffer.size(), 0.0);
		num_t *bufferPtr = buffer.data();
		for(size_t y=0;y<image._height;++y)
		{
			for(size_t x=0;x<image._width;++x)
			{
				image._dataPtr[y][x] = *bufferPtr;
				++bufferPtr;
			}
		}
		
		return image;
	} else {
		throw FitsIOException("No 2D images in HUD");
	}
}

long Image2D::GetImageCountInHUD(FitsFile &file) {
	int dimensions = file.GetCurrentImageDimensionCount();
	long total2DImageCount = 0;
	if(dimensions>=2) {
		total2DImageCount = 1;
		for(int j=3;j<=dimensions;j++) {
			total2DImageCount *= file.GetCurrentImageSize(j);
		}
	}
	return total2DImageCount;
}

void Image2D::SaveToFitsFile(const std::string &filename) const
{
	FitsFile file(filename);
	file.Create();
	file.AppendImageHUD(FitsFile::Double64ImageType, _width, _height);
	long bufferSize = (long) _width * (long) _height;
	double *buffer = new double[bufferSize];
	size_t i=0;
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x)
		{
			buffer[i] = _dataPtr[y][x];
			++i;
		}
	}
	try {
		file.WriteImage(0, buffer, bufferSize);
		file.Close();
	} catch(FitsIOException &exception) {
		delete[] buffer;
		throw;
	}
	delete[] buffer;
}

size_t Image2D::GetCountAbove(num_t value) const
{
	size_t count=0;
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x)
		{
			if(_dataPtr[y][x] > value) count++;
		}
	}
	return count;
}

num_t Image2D::GetTresholdForCountAbove(size_t count) const
{
	const size_t size = _width * _height;
	num_t *sorted = new num_t[size];
	size_t i = 0;
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x)
		{
			sorted[i] = _dataPtr[y][x];
			++i;
		}
	}
	std::sort(sorted, sorted + size);
	num_t v = sorted[size - count - 1];
	delete[] sorted;
	return v;
}

void Image2D::CopyData(num_t *destination) const
{
	size_t i=0;
	for(size_t y=0;y<_height;++y)
	{
		for(size_t x=0;x<_width;++x)
		{
			destination[i] = _dataPtr[y][x];
			++i;
		}
	}
}

void Image2D::MultiplyValues(num_t factor)
{
	const size_t size = _stride * _height;
	for(size_t i=0;i<size;++i)
	{
		_dataConsecutive[i] *= factor;
	}
}

void Image2D::SubtractAsRHS(const Image2DCPtr &lhs)
{
	float *thisPtr = &_dataConsecutive[0];
	const float *otherPtr = &(lhs->_dataConsecutive[0]);
	float *end = thisPtr + _stride * _height;
#ifdef USE_INTRINSICS
/* #ifdef __AVX__
	while(thisPtr < end)
	{
		// (*thisPtr) = (*otherPtr) - (*thisPtr);
		_mm_store256_ps(thisPtr, _mm_sub256_ps(_mm_load256_ps(otherPtr), _mm_load256_ps(thisPtr)));
		thisPtr += 8;
		otherPtr += 8;
	}
#else // Use slower SSE instructions
*/
	while(thisPtr < end)
	{
		// (*thisPtr) = (*otherPtr) - (*thisPtr);
		_mm_store_ps(thisPtr, _mm_sub_ps(_mm_load_ps(otherPtr), _mm_load_ps(thisPtr)));
		thisPtr += 4;
		otherPtr += 4;
	}
#else
	while(thisPtr < end)
	{
		(*thisPtr) = (*otherPtr) - (*thisPtr);
		thisPtr++;
		otherPtr++;
	}
#endif
}

Image2D Image2D::ShrinkHorizontally(size_t factor) const
{
	size_t newWidth = (_width + factor - 1) / factor;

	Image2D newImage(newWidth, _height);

	for(size_t x=0;x<newWidth;++x)
	{
		size_t binSize = factor;
		if(binSize + x*factor > _width)
			binSize = _width - x*factor;

		for(size_t y=0;y<_height;++y)
		{
			num_t sum = 0.0;
			for(size_t binX=0;binX<binSize;++binX)
			{
				size_t curX = x*factor + binX;
				sum += Value(curX, y);
			}
			newImage.SetValue(x, y, sum / (num_t) binSize);
		}
	}
	return newImage;
}

Image2D Image2D::ShrinkVertically(size_t factor) const
{
	size_t newHeight = (_height + factor - 1) / factor;

	Image2D newImage(_width, newHeight);

	for(size_t y=0;y<newHeight;++y)
	{
		size_t binSize = factor;
		if(binSize + y*factor > _height)
			binSize = _height - y*factor;

		for(size_t x=0;x<_width;++x)
		{
			num_t sum = 0.0;
			for(size_t binY=0;binY<binSize;++binY)
			{
				size_t curY = y*factor + binY;
				sum += Value(x, curY);
			}
			newImage.SetValue(x, y, sum / (num_t) binSize);
		}
	}
	return newImage;
}

Image2D Image2D::EnlargeHorizontally(size_t factor, size_t newWidth) const
{
	Image2D newImage(newWidth, _height);

	for(size_t x=0;x<newWidth;++x)
	{
		size_t xOld = x / factor;

		for(size_t y=0;y<_height;++y)
		{
			newImage.SetValue(x, y, Value(xOld, y));
		}
	}
	return newImage;
}

Image2D Image2D::EnlargeVertically(size_t factor, size_t newHeight) const
{
	Image2D newImage(_width, newHeight);

	for(size_t x=0;x<_width;++x)
	{
		for(size_t y=0;y<newHeight;++y)
		{
			size_t yOld = y / factor;
			newImage.SetValue(x, y, Value(x, yOld));
		}
	}
	return newImage;
}

Image2D Image2D::Trim(size_t startX, size_t startY, size_t endX, size_t endY) const
{
	size_t
		width = endX - startX,
		height = endY - startY;
	Image2D image(width, height);
	for(size_t y=startY;y<endY;++y)
	{
		num_t *newPtr = image._dataPtr[y-startY];
		num_t *oldPtr = &_dataPtr[y][startX];
		for(size_t x=startX;x<endX;++x)
		{
			*newPtr = *oldPtr;
			++newPtr;
			++oldPtr;
		}
	}
	return image;
}

void Image2D::SetTrim(size_t startX, size_t startY, size_t endX, size_t endY)
{
	*this = Trim(startX, startY, endX, endY);
}

/**
	* Returns the maximum value in the specified range.
	* @return The maximimum value.
	*/
num_t Image2D::GetMaximum(size_t xOffset, size_t yOffset, size_t width, size_t height) const
{
	size_t count = 0;
	num_t max =0.0;
	for(size_t y=yOffset;y<height+yOffset;++y) {
		for(size_t x=xOffset;x<width+xOffset;++x)
		{
			if(Value(x,y) > max || count==0)
			{
				max = Value(x, y);
				++count;
			}
		}
	}
	if(count == 0)
		return std::numeric_limits<num_t>::quiet_NaN();
	return max;
}

/**
	* Returns the minimum value in the specified range.
	* @return The minimum value.
	*/
num_t Image2D::GetMinimum(size_t xOffset, size_t yOffset, size_t width, size_t height) const
{
	size_t count = 0;
	num_t min = 0.0;
	for(size_t y=yOffset;y<height+yOffset;++y) {
		for(size_t x=xOffset;x<width+xOffset;++x)
		{
			if(Value(x,y) < min || count==0)
			{
				min = Value(x, y);
				++count;
			}
		}
	}
	if(count == 0)
		return std::numeric_limits<num_t>::quiet_NaN();
	return min;
}

void Image2D::ResizeWithoutReallocation(size_t newWidth)
{
	if(newWidth > _stride)
		throw BadUsageException("Bug: ResizeWithoutReallocation called with newWidth > Stride !");
	_width = newWidth;
}

Image2D& Image2D::operator+=(const Image2D& rhs)
{
	if(Width() != rhs.Width() || Height() != rhs.Height() || Stride() != rhs.Stride())
		throw BadUsageException("Images do not match in size");
	const size_t total = rhs._stride * rhs.Height();
	for(size_t i=0;i<total;++i) {
		_dataConsecutive[i] += rhs._dataConsecutive[i];
	}
	return *this;
}
