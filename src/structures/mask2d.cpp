#include "mask2d.h"
#include "image2d.h"

#include <iostream>

Mask2D::Mask2D(const Mask2D& source) : Mask2D(source.Width(), source.Height())
{
	memcpy(_valuesConsecutive, source._valuesConsecutive, _stride * _height * sizeof(bool));
}

Mask2D::Mask2D(Mask2D&& source) noexcept :
	_width(source._width),
	_height(source._height),
	_stride(source._stride),
	_values(source._values),
	_valuesConsecutive(source._valuesConsecutive)
{
	source._width = 0;
	source._stride = 0;
	source._height = 0;
	source._values = nullptr;
	source._valuesConsecutive = nullptr;
}

Mask2D::Mask2D(size_t width, size_t height) :
	_width(width),
	_height(height),
	_stride((((width-1)/4)+1)*4)
{
	if(_width == 0) _stride=0;
	allocate();
}

void Mask2D::allocate()
{
	unsigned allocHeight = ((((_height-1)/4)+1)*4);
	if(_height == 0) allocHeight = 0;
	_valuesConsecutive = new bool[_stride * allocHeight * sizeof(bool)];
	
	_values = new bool*[allocHeight];
	for(size_t y=0;y<_height;++y)
	{
		_values[y] = &_valuesConsecutive[_stride * y];
		// Even though the values after the requested width are never relevant, we will
		// initialize them to true to prevent valgrind to report unset values when they
		// are used in SSE instructions.
		for(size_t x=_width;x<_stride;++x)
		{
			_values[y][x] = true;
		}
	}
	for(size_t y=_height;y<allocHeight;++y)
	{
		_values[y] = &_valuesConsecutive[_stride * y];
		// (see remark above about initializing to true)
		for(size_t x=0;x<_stride;++x)
		{
			_values[y][x] = true;
		}
	}
}

Mask2D::~Mask2D() noexcept
{
	delete[] _values;
	delete[] _valuesConsecutive;
}

Mask2D& Mask2D::operator=(const Mask2D& rhs)
{
	if(_width != rhs._width ||
		_height != rhs._height ||
		_stride != rhs._stride)
	{
		delete[] _values;
		free(_valuesConsecutive);
		_width = rhs._width;
		_height = rhs._height;
		_stride = rhs._stride;
		allocate();
	}
	std::copy(rhs._valuesConsecutive, rhs._valuesConsecutive + _stride*_height, _valuesConsecutive);
	return *this;
}

Mask2D& Mask2D::operator=(Mask2D&& rhs) noexcept
{
	std::swap(rhs._width, _width);
	std::swap(rhs._stride, _stride);
	std::swap(rhs._height, _height);
	std::swap(rhs._values, _values);
	std::swap(rhs._valuesConsecutive, _valuesConsecutive);
	return *this;
}

Mask2D *Mask2D::CreateUnsetMask(const Image2D &templateImage)
{
	return new Mask2D(templateImage.Width(), templateImage.Height());
}

template <bool InitValue>
Mask2D *Mask2D::CreateSetMask(const class Image2D &templateImage)
{
	size_t
		width = templateImage.Width(),
		height = templateImage.Height();

	Mask2D *newMask = new Mask2D(width, height);
	memset(newMask->_valuesConsecutive, InitValue, newMask->_stride * height * sizeof(bool));
	return newMask;
}

template Mask2D *Mask2D::CreateSetMask<false>(const class Image2D &templateImage);
template Mask2D *Mask2D::CreateSetMask<true>(const class Image2D &templateImage);

Mask2D Mask2D::ShrinkHorizontally(int factor) const
{
	size_t newWidth = (_width + factor - 1) / factor;

	Mask2D newMask(newWidth, _height);

	for(size_t x=0;x<newWidth;++x)
	{
		size_t binSize = factor;
		if(binSize + x*factor > _width)
			binSize = _width - x*factor;

		for(size_t y=0;y<_height;++y)
		{
			bool value = false;
			for(size_t binX=0;binX<binSize;++binX)
			{
				size_t curX = x*factor + binX;
				value = value | Value(curX, y);
			}
			newMask.SetValue(x, y, value);
		}
	}
	return newMask;
}

Mask2D Mask2D::ShrinkHorizontallyForAveraging(int factor) const
{
	size_t newWidth = (_width + factor - 1) / factor;

	Mask2D newMask(newWidth, _height);

	for(size_t x=0;x<newWidth;++x)
	{
		size_t binSize = factor;
		if(binSize + x*factor > _width)
			binSize = _width - x*factor;

		for(size_t y=0;y<_height;++y)
		{
			bool value = true;
			for(size_t binX=0;binX<binSize;++binX)
			{
				size_t curX = x*factor + binX;
				value = value & Value(curX, y);
			}
			newMask.SetValue(x, y, value);
		}
	}
	return newMask;
}

Mask2D Mask2D::ShrinkVertically(int factor) const
{
	size_t newHeight = (_height + factor - 1) / factor;

	Mask2D newMask(_width, newHeight);

	for(size_t y=0;y<newHeight;++y)
	{
		size_t binSize = factor;
		if(binSize + y*factor > _height)
			binSize = _height - y*factor;

		for(size_t x=0;x<_width;++x)
		{
			bool value = false;
			for(size_t binY=0;binY<binSize;++binY)
			{
				size_t curY = y*factor + binY;
				value = value | Value(x, curY);
			}
			newMask.SetValue(x, y, value);
		}
	}
	return newMask;
}

void Mask2D::EnlargeHorizontallyAndSet(const Mask2D& smallMask, int factor)
{
	for(size_t x=0;x<smallMask.Width();++x)
	{
		size_t binSize = factor;
		if(binSize + x*factor > _width)
			binSize = _width - x*factor;

		for(size_t y=0;y<_height;++y)
		{
			for(size_t binX=0;binX<binSize;++binX)
			{
				size_t curX = x*factor + binX;
				SetValue(curX, y, smallMask.Value(x, y));
			}
		}
	}
}

void Mask2D::EnlargeVerticallyAndSet(const Mask2D& smallMask, int factor)
{
	for(size_t y=0;y<smallMask.Height();++y)
	{
		size_t binSize = factor;
		if(binSize + y*factor > _height)
			binSize = _height - y*factor;

		for(size_t x=0;x<_width;++x)
		{
			for(size_t binY=0;binY<binSize;++binY)
			{
				size_t curY = y*factor + binY;
				SetValue(x, curY, smallMask.Value(x, y));
			}
		}
	}
}
