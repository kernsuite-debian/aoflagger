#ifndef MASK2D_H
#define MASK2D_H

#include <cstring>
#include <memory>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include "image2d.h"

typedef boost::intrusive_ptr<class Mask2D> Mask2DPtr;
typedef boost::intrusive_ptr<const class Mask2D> Mask2DCPtr;

void swap(Mask2D&, Mask2D&);
void swap(Mask2D&, Mask2D&&);
void swap(Mask2D&&, Mask2D&);

class Mask2D : public boost::intrusive_ref_counter<Mask2D> {
	public:
		Mask2D(const Mask2D& source);
		
		Mask2D(Mask2D&& source) noexcept;
		
		~Mask2D() noexcept;

		Mask2D& operator=(const Mask2D& rhs);
		
		Mask2D& operator=(Mask2D&& rhs) noexcept;
		
		bool operator==(const Mask2D& rhs) const
		{
			if(_width != rhs._width || _height != rhs._height)
				return false;
			for(size_t y=0;y<_height;++y)
			{
				for(size_t x=0;x<_width;++x)
					if(_values[y][x] != rhs._values[y][x])
						return false;
			}
			return true;
		}
		
		bool operator!=(const Mask2D& rhs) const { return !(*this == rhs); }

		template<typename... Args>
		static Mask2DPtr MakePtr(Args&&... args)
		{
			// This function is to have a generic 'make_<ptr>' function, that e.g. calls
			// the more efficient make_shared() when Mask2DPtr is a shared_ptr, but
			// also works when Mask2DPtr is a boost::intrusive_ptr.
			return Mask2DPtr(new Mask2D(std::forward<Args>(args)...));
		}
		
		static Mask2D MakeUnsetMask(size_t width, size_t height)
		{
			return Mask2D(width, height);
		}
		
		template<bool InitValue>
		static Mask2D MakeSetMask(size_t width, size_t height)
		{
			Mask2D newMask(width, height);
			memset(newMask._valuesConsecutive, InitValue, newMask._stride * height * sizeof(bool));
			return newMask;
		}
		
		static Mask2D *CreateUnsetMask(size_t width, size_t height)
		{
			return new Mask2D(width, height);
		}
		static Mask2DPtr CreateUnsetMaskPtr(size_t width, size_t height)
		{
			return Mask2DPtr(new Mask2D(width, height));
		}

		static Mask2D *CreateUnsetMask(const class Image2D &templateImage);
		static Mask2DPtr CreateUnsetMask(Image2DCPtr templateImage)
		{
			return Mask2DPtr(CreateUnsetMask(*templateImage));
		}

		template<bool InitValue>
		static Mask2D *CreateSetMask(const class Image2D &templateImage);

		template<bool InitValue>
		static Mask2DPtr CreateSetMask(Image2DCPtr templateImage)
		{
			return Mask2DPtr(CreateSetMask<InitValue>(*templateImage));
		}

		template<bool InitValue>
		static Mask2D *CreateSetMask(size_t width, size_t height)
		{
			Mask2D *newMask = new Mask2D(width, height);
			memset(newMask->_valuesConsecutive, InitValue, newMask->_stride * height * sizeof(bool));
			return newMask;
		}

		template<bool InitValue>
		static Mask2DPtr CreateSetMaskPtr(size_t width, size_t height)
		{
			return Mask2DPtr(CreateSetMask<InitValue>(width, height));
		}

		bool Value(size_t x, size_t y) const
		{
			return _values[y][x];
		}
		
		void SetValue(size_t x, size_t y, bool newValue)
		{
			_values[y][x] = newValue;
		}
		
		void SetHorizontalValues(size_t x, size_t y, bool newValue, size_t count)
		{
			memset(&_values[y][x], newValue, count * sizeof(bool));
		}
		
		void SetVerticalValues(size_t x, size_t y, bool newValue, size_t count)
		{
			for(size_t i=0; i!=count; ++i)
				_values[y + i][x] = newValue;;
		}
		
		size_t Width() const { return _width; }
		
		size_t Height() const { return _height; }

		bool AllFalse() const
		{
			for(size_t y=0;y<_height;++y)
			{
				for(size_t x=0;x<_width;++x)
				{
					if(_values[y][x])
						return false;
				}
			}
			return true;
		}

		/**
		 * Returns a pointer to one row of data. This can be used to step
		 * quickly over the data in x direction. Note that the next row
		 * is not exactly at "one times width", because the number of
		 * samples in a row is made divisable by four. This makes it
		 * possible to execute SSE instruction easily.
		 * 
		 * If you want to skip over a whole row, use the Stride() method
		 * to determine the intrinsicly used width of one row.
		 * 
		 * @see Stride()
		 */
		bool *ValuePtr(size_t x, size_t y)
		{
			return &_values[y][x];
		}
		
		/**
		 * Returns a constant pointer to one row of data. This can be used to
		 * step quickly over the data in x direction. Note that the next row
		 * is not exactly at "one times width", because the number of
		 * samples in a row is made divisable by four. This makes it
		 * possible to execute SSE instruction easily.
		 * 
		 * If you want to skip over a whole row, use the Stride() method
		 * to determine the intrinsicly used width of one row.
		 * 
		 * @see Stride()
		 */
		const bool *ValuePtr(size_t x, size_t y) const
		{
			return &_values[y][x];
		}
		
		bool *Data()
		{
			return _valuesConsecutive;
		}
		
		const bool *Data() const
		{
			return _valuesConsecutive;
		}
		
		/**
		 * This value specifies the intrinsic width of one row. It is
		 * normally the first number that is >= Width() and divisable by
		 * four. When using the ValuePtr(unsigned, unsigned) method,
		 * this value can be used to step over one row.
		 * 
		 * @see ValuePtr(unsigned, unsigned)
		 */
		size_t Stride() const
		{
			return _stride;
		}
		
		template<bool NewValue>
		void SetAll()
		{
			memset(_valuesConsecutive, NewValue, _stride  * _height * sizeof(bool));
		}

		template<bool NewValue>
		void SetAllVertically(size_t x)
		{
			for(size_t y=0;y<_height;++y)
				_values[y][x] = NewValue;
		}

		template<bool NewValue>
		void SetAllVertically(size_t startX, size_t endX)
		{
			for(size_t x=startX;x<endX;++x)
			{
				for(size_t y=0;y<_height;++y)
					_values[y][x] = NewValue;
			}
		}

		template<bool NewValue>
		void SetAllHorizontally(size_t y)
		{
			memset(_values[y], NewValue, _width * sizeof(bool));
		}

		template<bool BoolValue>
		void SetAllHorizontally(size_t startY, size_t endY)
		{
			memset(_values[startY], BoolValue, _width * sizeof(bool) * (endY - startY));
		}

		void Invert()
		{
			for(size_t y=0;y<_height;++y)
			{
				for(size_t x=0;x<_width;++x)
					_values[y][x] = !_values[y][x];
			}
		}
		
		/**
		 * Flips the image round the diagonal, i.e., x becomes y and y becomes x.
		 */
		Mask2D MakeXYFlipped() const
		{
			Mask2D mask(_height, _width);
			for(size_t y=0;y<_height;++y)
			{
				for(size_t x=0;x<_width;++x)
					mask._values[x][y] = _values[y][x];
			}
			return mask;
		}

		template<bool BoolValue>
		size_t GetCount() const
		{
			size_t count = 0;
			for(size_t y=0;y<_height;++y)
			{
				for(size_t x=0;x<_width;++x)
					if(BoolValue == _values[y][x])
						++count;
			}
			return count;
		}
		
		Mask2D ShrinkHorizontally(int factor) const;
		Mask2D ShrinkHorizontallyForAveraging(int factor) const;
		
		Mask2D ShrinkVertically(int factor) const;
		Mask2D ShrinkVerticallyForAveraging(int factor) const;

		void EnlargeHorizontallyAndSet(const Mask2D& smallMask, int factor);
		void EnlargeVerticallyAndSet(const Mask2D& smallMask, int factor);

		void Join(const Mask2D& other)
		{
			for(size_t y=0;y<_height;++y) {
				for(size_t x=0;x<_width;++x)
					SetValue(x, y, other.Value(x, y) || Value(x, y));
			}
		}
		
		void Intersect(const Mask2D& other)
		{
			for(size_t y=0;y<_height;++y) {
				for(size_t x=0;x<_width;++x)
					SetValue(x, y, other.Value(x, y) && Value(x, y));
			}
		}
		
		Mask2D Trim(size_t startX, size_t startY, size_t endX, size_t endY) const
		{
			size_t
				width = endX - startX,
				height = endY - startY;
			Mask2D mask(width, height);
			for(size_t y=startY;y<endY;++y)
			{
				for(size_t x=startX;x<endX;++x)
					mask.SetValue(x-startX, y-startY, Value(x, y));
			}
			return mask;
		}
		
		void CopyFrom(const Mask2D& source, size_t destX, size_t destY)
		{
			size_t
				x2 = source._width + destX,
				y2 = source._height + destY;
			if(x2 > _width) x2 = _width;
			if(y2 > _height) y2 = _height;
			for(size_t y=destY;y<y2;++y)
			{
				for(size_t x=destX;x<x2;++x)
					SetValue(x, y, source.Value(x-destX, y-destY));
			}
		}
		
		void SwapXY()
		{
			Mask2D tempMask(_height, _width);
			for(size_t y=0;y<_height;++y)
			{
				for(size_t x=0;x<_width;++x)
				{
					tempMask.SetValue(y, x, Value(x, y));
				}
			}
			*this = std::move(tempMask);
		}
	private:
		friend void swap(Mask2D&, Mask2D&);
		friend void swap(Mask2D&, Mask2D&&);
		friend void swap(Mask2D&&, Mask2D&);
		
		Mask2D(size_t width, size_t height);
		
		void allocate();
		
		size_t _width, _height;
		size_t _stride;
		
		bool **_values;
		bool *_valuesConsecutive;
};

inline void swap(Mask2D& left, Mask2D& right)
{
	std::swap(left._width, right._width);
	std::swap(left._stride, right._stride);
	std::swap(left._height, right._height);
	std::swap(left._values, right._values);
	std::swap(left._valuesConsecutive, right._valuesConsecutive);
}

inline void swap(Mask2D& left, Mask2D&& right)
{
	std::swap(left._width, right._width);
	std::swap(left._stride, right._stride);
	std::swap(left._height, right._height);
	std::swap(left._values, right._values);
	std::swap(left._valuesConsecutive, right._valuesConsecutive);
}

inline void swap(Mask2D&& left, Mask2D& right)
{
	std::swap(left._width, right._width);
	std::swap(left._stride, right._stride);
	std::swap(left._height, right._height);
	std::swap(left._values, right._values);
	std::swap(left._valuesConsecutive, right._valuesConsecutive);
}

#endif
